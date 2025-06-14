/**
 * dcs Drive Control System - Base.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "dcs.h"
#include "core1_main.h"

#include "board.h"
#include "debug_support.h"

#include "cmt/cmt.h"
#include "hid/hid.h"
#include "util/util.h"

#include "lib/json-maker/json-maker.h"

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define DCS_STATUS_PERIOD 313 // Every 5 seconds (313 * 0.016 = 5.008)
#define DCS_HOST_STATUS_PERIOD 938  // Send status to host every 15 seconds

static bool _dcs_initialized = false;
static bool _hwrt_started = false;

static int _dcs_hk_cnt;


// Message handler functions...
static void _handle_dcs_housekeeping(cmt_msg_t* msg);
static void _handle_dcs_test(cmt_msg_t* msg);
static void _handle_hwrt_started(cmt_msg_t* msg);

// Idle functions...
static void _dcs_idle_function_1();
static void _dcs_idle_function_2();

// Hardware functions...

// Internal functions
static void _dcs_started();


// ====================================================================
// Message handler functions
// ====================================================================

/**
 * @brief Handle DCS Housekeeping. Triggered at ~16ms intervals.
 *
 * @param msg Nothing important in the message.
 */
static void _handle_dcs_housekeeping(cmt_msg_t* msg) {
    // Do any regular status updates, cleanup, etc.

    // We do status updates at certain periods, and we
    // offset different operations a bit, just so not to
    // do too much all in one time slot.
    if (++_dcs_hk_cnt % DCS_STATUS_PERIOD == 0) {
        debug_printf("DCS: %d\n", _dcs_hk_cnt);
    }
    if ((_dcs_hk_cnt % DCS_HOST_STATUS_PERIOD) == 0) {
        // Send our status to the host.
        //printf("DCS: %d A:%d B:%d\n", _dcs_hk_cnt, aon, bon);
    }
}

static void _handle_dcs_test(cmt_msg_t* msg) {
    // Test `scheduled_msg_ms` error
    static int times = 1;
    
    cmt_msg_t msg_time = { MSG_DCS_TEST, MSG_PRI_NORM };
    uint64_t period = 60;

    if (debug_mode_enabled()) {
        uint64_t now = now_us();

        uint64_t last_time = msg->data.ts_us;
        int64_t error = ((now - last_time) - (period * 1000 * 1000));
        float error_per_ms = ((error * 1.0) / (period * 1000.0));
        info_printf("\n%5.5d - Scheduled msg delay error us/ms:%5.2f\n", times, error_per_ms);
    }
    msg_time.data.ts_us = now_us(); // Get the 'next' -> 'last_time' fresh
    schedule_msg_in_ms((period * 1000), &msg_time);
    times++;
}

static void _handle_hwrt_started(cmt_msg_t* msg) {
    // The Hardware Operating System has reported that it is started.
    // Since we are responding to a message, it means we
    // are also initialized, so -
    //
    // Start things running.
    _hwrt_started = true;
    _dcs_started();
}


// ====================================================================
// Local functions
// ====================================================================

/**
 * @brief Called after the DCS is started - the message loop is running.
 *
 * Initialization of DCS modules that require the message loop
 * should be put here. Modules that need to be initialized before the
 * message loop is running should go in `dcs_module_init`.
 */
static void _dcs_started() {
    static bool _started = false;
    if (_started) {
        board_panic("_dcs_started - Called more than once.");
    }

    cmt_msg_hdlr_add(MSG_HOUSEKEEPING_RT, _handle_dcs_housekeeping);
    cmt_msg_hdlr_add(MSG_DCS_TEST, _handle_dcs_test);

    // Let the HW level know that we are started.
    cmt_msg_t msg;
    cmt_msg_init(&msg, MSG_DCS_STARTED);
    postHWRTMsg(&msg);
}



// ====================================================================
// Drive Control System functions
// ====================================================================



// ====================================================================
// Initialization functions
// ====================================================================


void dcs_module_init() {
    if (_dcs_initialized) {
        board_panic("dcs_module_init called multiple times");
    }

    _dcs_initialized = true;
    _dcs_hk_cnt = 0;
}

void start_dcs() {
    static bool _started = false;
    // Make sure we aren't already started and that we are being called from core-0.
    assert(!_started && 0 == get_core_num());
    _started = true;
    // Register our handler for the Hardware Control Runtime started, saying that it is for the DCS Core.
    cmt_msg_hdlr_add_for_core(MSG_HWRT_STARTED, _handle_hwrt_started, DCS_CORE_NUM);

    start_core1(); // The Core-1 main starts the DCS
}
