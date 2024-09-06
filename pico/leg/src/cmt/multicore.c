/**
 * leg Multicore common.
 *
 * Contains the data structures and routines to handle multicore functionality.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "multicore.h"
#include "system_defs.h"
#include "cmt.h"
#include "board.h"
#include "debug_support.h"

#include <stdio.h>
#include <string.h>

#define CORE0_QUEUE_ENTRIES_MAX 32
#define CORE0_HP_QUEUE_ENTRIES_MAX 8
#define CORE1_QUEUE_ENTRIES_MAX 32
#define CORE1_HP_QUEUE_ENTRIES_MAX 8

static bool _initialized = false;

queue_t core0_queue;
queue_t core0_queue_hp;
queue_t core1_queue;
queue_t core1_queue_hp;

void get_core0_msg_blocking(cmt_msg_t* msg) {
    register bool retrieved = false;
    uint32_t flags = save_and_disable_interrupts();
    retrieved = queue_try_remove(&core0_queue_hp, msg);
    restore_interrupts(flags);
    if (!retrieved) {
        queue_remove_blocking(&core0_queue, msg);
    }
}

bool get_core0_msg_nowait(cmt_msg_t* msg) {
    register bool retrieved = false;
    uint32_t flags = save_and_disable_interrupts();
    retrieved = queue_try_remove(&core0_queue_hp, msg);
    if (!retrieved) {
        retrieved = queue_try_remove(&core0_queue, msg);
    }
    restore_interrupts(flags);

    return (retrieved);
}

void get_core1_msg_blocking(cmt_msg_t* msg) {
    register bool retrieved = false;
    uint32_t flags = save_and_disable_interrupts();
    retrieved = queue_try_remove(&core1_queue_hp, msg);
    restore_interrupts(flags);
    if (!retrieved) {
        queue_remove_blocking(&core1_queue, msg);
    }
}

bool get_core1_msg_nowait(cmt_msg_t* msg) {
    register bool retrieved = false;
    uint32_t flags = save_and_disable_interrupts();
    retrieved = queue_try_remove(&core1_queue_hp, msg);
    if (!retrieved) {
        retrieved = queue_try_remove(&core1_queue, msg);
    }
    restore_interrupts(flags);

    return (retrieved);
}

void multicore_module_init() {
    assert(!_initialized);
    _initialized = true;
    queue_init(&core0_queue, sizeof(cmt_msg_t), CORE0_QUEUE_ENTRIES_MAX);
    queue_init(&core0_queue_hp, sizeof(cmt_msg_t), CORE0_HP_QUEUE_ENTRIES_MAX);
    queue_init(&core1_queue, sizeof(cmt_msg_t), CORE1_QUEUE_ENTRIES_MAX);
    queue_init(&core1_queue_hp, sizeof(cmt_msg_t), CORE1_HP_QUEUE_ENTRIES_MAX);
    cmt_module_init();
}

static void _check_q0_level(int id) {
    if (debug_mode_enabled()) {
        uint level = queue_get_level(&core0_queue);
        if (CORE0_QUEUE_ENTRIES_MAX - level < 2) {
            cmt_msg_t msg;
            if (queue_try_peek(&core0_queue, &msg)) {
                printf("\n!!! Q0 level %u - Head Msg:%#04.4x !!!", level, msg.id);
            }
        }
    }
}

static void _check_q0_hp_level(int id) {
    if (debug_mode_enabled()) {
        uint level = queue_get_level(&core0_queue_hp);
        if (CORE0_HP_QUEUE_ENTRIES_MAX - level < 2) {
            cmt_msg_t msg;
            if (queue_try_peek(&core0_queue_hp, &msg)) {
                printf("\n!!! Q0 HP level %u - Head Msg:%#04.4x !!!", level, msg.id);
            }
        }
    }
}

static void _check_q1_level(int id) {
    if (debug_mode_enabled()) {
        uint level = queue_get_level(&core1_queue);
        if (CORE1_QUEUE_ENTRIES_MAX - level < 2) {
            cmt_msg_t msg;
            if (queue_try_peek(&core1_queue, &msg)) {
                printf("\n!!! Q1 level %u - Head Msg:%#04.4x !!!", level, msg.id);
            }
        }
    }
}

static void _check_q1_hp_level(int id) {
    if (debug_mode_enabled()) {
        uint level = queue_get_level(&core1_queue_hp);
        if (CORE1_HP_QUEUE_ENTRIES_MAX - level < 2) {
            cmt_msg_t msg;
            if (queue_try_peek(&core1_queue_hp, &msg)) {
                printf("\n!!! Q1 HP level %u - Head Msg:%#04.4x !!!", level, msg.id);
            }
        }
    }
}

void post_by_id_to_core0_blocking(msg_id_t msg_id, msg_priority_t priority) {
    cmt_msg_t msg;
    msg.id = msg_id;
    post_to_core0_blocking(&msg, priority);
}

void post_to_core0_blocking(const cmt_msg_t *msg, msg_priority_t priority) {
    if (priority > MSG_PRI_HIGH_FALLBACK) {
        _check_q0_hp_level(msg->id);
    }
    else {
        _check_q0_level(msg->id);
    }
    uint32_t flags = save_and_disable_interrupts();
    if (priority > MSG_PRI_NORMAL) {
        if (priority > MSG_PRI_HIGH_FALLBACK) {
            queue_add_blocking(&core0_queue_hp, msg);  // No fallback, so HP or block
        }
        else {
            bool posted = queue_try_add(&core0_queue_hp, msg);
            if (!posted) {
                queue_add_blocking(&core0_queue, msg);
            }
        }
    }
    else {
        queue_add_blocking(&core0_queue, msg);
    }
    restore_interrupts(flags);
}

bool post_by_id_to_core0_nowait(msg_id_t msg_id) {
    cmt_msg_t msg;
    msg.id = msg_id;
    return post_to_core0_nowait(&msg);
}

bool post_to_core0_nowait(const cmt_msg_t *msg) {
    _check_q0_level(msg->id);
    register bool posted = false;
    uint32_t flags = save_and_disable_interrupts();
    posted = queue_try_add(&core0_queue, msg);
    restore_interrupts(flags);

    return (posted);
}

void post_by_id_to_core1_blocking(msg_id_t msg_id, msg_priority_t priority) {
    cmt_msg_t msg;
    msg.id = msg_id;
    post_to_core1_blocking(&msg, priority);
}

void post_to_core1_blocking(const cmt_msg_t* msg, msg_priority_t priority) {
    if (priority > MSG_PRI_HIGH_FALLBACK) {
        _check_q1_hp_level(msg->id);
    }
    else {
        _check_q1_level(msg->id);
    }
    uint32_t flags = save_and_disable_interrupts();
    if (priority > MSG_PRI_NORMAL) {
        if (priority > MSG_PRI_HIGH_FALLBACK) {
            queue_add_blocking(&core1_queue_hp, msg);  // No fallback, so HP or block
        }
        else {
            bool posted = queue_try_add(&core1_queue_hp, msg);
            if (!posted) {
                queue_add_blocking(&core1_queue, msg);
            }
        }
    }
    else {
        queue_add_blocking(&core1_queue, msg);
    }
    restore_interrupts(flags);
}

bool post_by_id_to_core1_nowait(msg_id_t msg_id) {
    cmt_msg_t msg;
    msg.id = msg_id;
    return post_to_core1_nowait(&msg);
}

bool post_to_core1_nowait(const cmt_msg_t* msg) {
    _check_q1_level(msg->id);
    register bool posted = false;
    uint32_t flags = save_and_disable_interrupts();
    posted = queue_try_add(&core1_queue, msg);
    restore_interrupts(flags);

    return (posted);
}

void post_by_id_to_cores_blocking(msg_id_t msg_id, msg_priority_t priority) {
    cmt_msg_t msg;
    msg.id = msg_id;
    post_to_cores_blocking(&msg, priority);
}

void post_to_cores_blocking(const cmt_msg_t* msg, msg_priority_t priority) {
    post_to_core0_blocking(msg, priority);
    post_to_core1_blocking(msg, priority);
}

uint16_t post_by_id_to_cores_nowait(msg_id_t msg_id) {
    cmt_msg_t msg;
    msg.id = msg_id;
    return (post_to_cores_nowait(&msg));
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
