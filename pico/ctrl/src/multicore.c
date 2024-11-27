/**
 * hwctrl Multicore common.
 *
 * Contains the data structures and routines to handle multicore functionality.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "multicore.h"

#include "system_defs.h"
#include "board.h"
#include "debug_support.h"

#include "cmt/cmt_t.h"
#include "dcs/core1_main.h"

#include <stdio.h>
#include <string.h>

#define CORE0_QUEUE_NP_ENTRIES_MAX 64
#define CORE0_QUEUE_L9_ENTRIES_MAX 8
#define CORE0_QUEUE_LP_ENTRIES_MAX 8
#define CORE1_QUEUE_NP_ENTRIES_MAX 64
#define CORE1_QUEUE_L9_ENTRIES_MAX 8
#define CORE1_QUEUE_LP_ENTRIES_MAX 8

static int32_t _msg_num;
// Flag indicating that we don't want to panic if we can't add a message to a queue.
static bool    _no_qadd_panic;
static int     _c0_reqmsg_post_errs;
static int     _c1_reqmsg_post_errs;

static queue_t _core0_np_queue;
static queue_t _core0_l9_queue;
static queue_t _core0_lp_queue;
static queue_t _core1_np_queue;
static queue_t _core1_l9_queue;
static queue_t _core1_lp_queue;

static bool _all_q0_mt() {
    register uint level = queue_get_level(&_core0_l9_queue);
    if (level) {
        return false;
    }
    level = queue_get_level(&_core0_np_queue);
    if (level) {
        return false;
    }
    level = queue_get_level(&_core0_lp_queue);
    if (level) {
        return false;
    }
    return true;
}

static bool _all_q1_mt() {
    register uint level = queue_get_level(&_core1_l9_queue);
    if (level) {
        return false;
    }
    level = queue_get_level(&_core1_np_queue);
    if (level) {
        return false;
    }
    level = queue_get_level(&_core1_lp_queue);
    if (level) {
        return false;
    }
    return true;
}

static void _copy_and_set_num_ts(cmt_msg_t* msg, const cmt_msg_t* msgsrc) {
    memcpy(msg, msgsrc, sizeof(cmt_msg_t));
    msg->n = ++_msg_num;
    msg->t = now_ms();
}


void get_core0_msg_blocking(cmt_msg_t* msg) {
    // Get a message from the L9 queue if exist, then try the NP, then LP.
    // If no messages exist, block on L9.
    register bool retrieved = false;
    uint32_t flags = save_and_disable_interrupts();
    retrieved = queue_try_remove(&_core0_l9_queue, msg);
    if (retrieved) {
        restore_interrupts(flags);
        return;
    }
    retrieved = queue_try_remove(&_core0_np_queue, msg);
    if (retrieved) {
        restore_interrupts(flags);
        return;
    }
    retrieved = queue_try_remove(&_core0_lp_queue, msg);
    if (retrieved) {
        restore_interrupts(flags);
        return;
    }
    // No messages existed. Restore interrupts and block on the L9 queue.
    restore_interrupts(flags);
    queue_remove_blocking(&_core0_l9_queue,msg);
}

bool get_core0_msg_nowait(cmt_msg_t* msg) {
    // Get a message from the L9 queue if exist, then try the NP, then LP.
    // If no messages exist return false.
    register bool retrieved = false;
    uint32_t flags = save_and_disable_interrupts();
    retrieved = queue_try_remove(&_core0_l9_queue, msg);
    if (retrieved) {
        restore_interrupts(flags);
        return (retrieved);
    }
    retrieved = queue_try_remove(&_core0_np_queue, msg);
    if (retrieved) {
        restore_interrupts(flags);
        return (retrieved);
    }
    retrieved = queue_try_remove(&_core0_lp_queue, msg);
    restore_interrupts(flags);
    return (retrieved);
}

void get_core1_msg_blocking(cmt_msg_t* msg) {
    // Get a message from the L9 queue if exist, then try the NP, then LP.
    // If no messages exist, block on L9.
    register bool retrieved = false;
    uint32_t flags = save_and_disable_interrupts();
    retrieved = queue_try_remove(&_core1_l9_queue, msg);
    if (retrieved) {
        restore_interrupts(flags);
        return;
    }
    retrieved = queue_try_remove(&_core1_np_queue, msg);
    if (retrieved) {
        restore_interrupts(flags);
        return;
    }
    retrieved = queue_try_remove(&_core1_lp_queue, msg);
    if (retrieved) {
        restore_interrupts(flags);
        return;
    }
    // No messages existed. Restore interrupts and block on the L9 queue.
    restore_interrupts(flags);
    queue_remove_blocking(&_core1_l9_queue, msg);
}

bool get_core1_msg_nowait(cmt_msg_t* msg) {
    // Get a message from the L9 queue if exist, then try the NP, then LP.
    // If no messages exist return false.
    register bool retrieved = false;
    uint32_t flags = save_and_disable_interrupts();
    retrieved = queue_try_remove(&_core1_l9_queue, msg);
    if (retrieved) {
        restore_interrupts(flags);
        return (retrieved);
    }
    retrieved = queue_try_remove(&_core1_np_queue, msg);
    if (retrieved) {
        restore_interrupts(flags);
        return (retrieved);
    }
    retrieved = queue_try_remove(&_core1_lp_queue, msg);
    restore_interrupts(flags);
    return (retrieved);
}

void post_to_core0(const cmt_msg_t* msg) {
    // Post to the appropriate queue based on the priority -
    // however, if all queues are empty, post to the L9 queue, as that
    // is the queue that will be blocked on by the 'get' method.
    queue_t* q2use = &_core0_l9_queue;
    uint32_t flags = save_and_disable_interrupts();
    if (!_all_q0_mt()) {
        switch (msg->priority) {
        case MSG_PRI_NORM:
            q2use = &_core0_np_queue;
            break;
        case MSG_PRI_LP:
            q2use = &_core0_lp_queue;
            break;
        default:
            break;
        }
    }
    cmt_msg_t m; // queue_add copies the contents, so on the stack is okay.
    _copy_and_set_num_ts(&m, msg);
    register bool posted = queue_try_add(q2use, &m);
    restore_interrupts(flags);
    if (!posted) {
        _c0_reqmsg_post_errs++;
        if (!_no_qadd_panic) {
            panic("Req C0 msg could not post");
        }
    }
}

bool post_to_core0_nowait(const cmt_msg_t* msg) {
    cmt_msg_t m; // queue_add copies the contents, so on the stack is okay.
    _copy_and_set_num_ts(&m, msg);
    register bool posted = false;
    uint32_t flags = save_and_disable_interrupts();
    posted = queue_try_add(&_core0_np_queue, &m);
    restore_interrupts(flags);

    return (posted);
}

void post_to_core1(const cmt_msg_t* msg) {
    // Post to the appropriate queue based on the priority -
    // however, if all queues are empty, post to the L9 queue, as that
    // is the queue that will be blocked on by the 'get' method.
    queue_t* q2use = &_core1_l9_queue;
    uint32_t flags = save_and_disable_interrupts();
    if (!_all_q1_mt()) {
        switch (msg->priority) {
        case MSG_PRI_NORM:
            q2use = &_core1_np_queue;
            break;
        case MSG_PRI_LP:
            q2use = &_core1_lp_queue;
            break;
        default:
            break;
        }
    }
    cmt_msg_t m; // queue_add copies the contents, so on the stack is okay.
    _copy_and_set_num_ts(&m, msg);
    register bool posted = queue_try_add(q2use, &m);
    restore_interrupts(flags);
    if (!posted) {
        _c1_reqmsg_post_errs++;
        if (!_no_qadd_panic) {
            panic("Req C1 msg could not post");
        }
    }
}

bool post_to_core1_nowait(const cmt_msg_t* msg) {
    cmt_msg_t m; // queue_add copies the contents, so on the stack is okay.
    _copy_and_set_num_ts(&m, msg);
    register bool posted = false;
    uint32_t flags = save_and_disable_interrupts();
    posted = queue_try_add(&_core1_lp_queue, &m);
    restore_interrupts(flags);

    return (posted);
}

uint16_t post_to_cores_nowait(const cmt_msg_t* msg) {
    uint16_t retval = 0;
    if (post_to_core0_nowait(msg)) {
        retval |= 0x01;
    }
    if (post_to_core1_nowait(msg)) {
        retval |= 0x02;
    }
    return (retval);
}

void start_core1() {
    // Start up the Core 1 main.
    //
    // Core 1 must be started before FIFO interrupts are enabled.
    // (core1 launch uses the FIFO's, so enabling interrupts before
    // the FIFO's are used for the launch will result in unexpected behaviour.
    //
    multicore_launch_core1(core1_main);
}

void multicore_module_init(bool no_qadd_panic) {
    static bool _initialized = false;
    if (_initialized) {
        panic("Multicore already initialized");
    }
    _initialized = true;
    _msg_num = 0;
    _no_qadd_panic = no_qadd_panic;
    _c0_reqmsg_post_errs = 0;
    _c1_reqmsg_post_errs = 0;
    queue_init(&_core0_np_queue, sizeof(cmt_msg_t), CORE0_QUEUE_NP_ENTRIES_MAX);
    queue_init(&_core0_l9_queue, sizeof(cmt_msg_t), CORE0_QUEUE_L9_ENTRIES_MAX);
    queue_init(&_core0_lp_queue, sizeof(cmt_msg_t), CORE0_QUEUE_LP_ENTRIES_MAX);
    queue_init(&_core1_np_queue, sizeof(cmt_msg_t), CORE1_QUEUE_NP_ENTRIES_MAX);
    queue_init(&_core1_l9_queue, sizeof(cmt_msg_t), CORE1_QUEUE_L9_ENTRIES_MAX);
    queue_init(&_core1_lp_queue, sizeof(cmt_msg_t), CORE1_QUEUE_LP_ENTRIES_MAX);
}

