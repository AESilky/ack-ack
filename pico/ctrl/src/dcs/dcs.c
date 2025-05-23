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
#include "cmt/cmt_mh.h"
#include "hid/hid.h"
#include "sensbank/sensbank.h"
#include "util/util.h"

#include "lib/json-maker/json-maker.h"

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define DCS_STATUS_PERIOD 313 // Every 5 seconds (313 * 0.016 = 5.008)
#define DCS_HOST_STATUS_PERIOD 938  // Send status to host every 15 seconds

static bool _dcs_initialized = false;
static bool _hwos_started = false;

static int _dcs_hk_cnt;

// Message handler functions...
static void _handle_dcs_housekeeping(cmt_msg_t* msg);
static void _handle_dcs_test(cmt_msg_t* msg);
static void _handle_hwos_started(cmt_msg_t* msg);
static void _handle_sensbank_chg(cmt_msg_t* msg);

// Idle functions...
static void _dcs_idle_function_1();
static void _dcs_idle_function_2();

// Internal functions
static void _dcs_started();


static const msg_handler_entry_t _dcs_housekeeping_he = { MSG_HOUSEKEEPING_RT, _handle_dcs_housekeeping };
static const msg_handler_entry_t _dcs_test_he = { MSG_DCS_TEST, _handle_dcs_test };
static const msg_handler_entry_t _hwos_started_he = { MSG_HWOS_STARTED, _handle_hwos_started };
static const msg_handler_entry_t _sbchg_he = { MSG_SENSBANK_CHG, _handle_sensbank_chg };

// For performance - put these in order that we expect to receive more often
static const msg_handler_entry_t* _dcs_handler_entries[] = {
    & _dcs_housekeeping_he,
    & cmt_sm_sleep_handler_entry,    // CMT Scheduled Message 'Sleep' handler
    & _sbchg_he,
    & _dcs_test_he,
    & _hwos_started_he,
    ((msg_handler_entry_t*)0), // Last entry must be a NULL
};

msg_loop_cntx_t dcs_msg_loop_cntx = {
    DCS_CORE_NUM, // Drive Control System runs on Core 1
    _dcs_handler_entries,
};

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
    static bool aon = false;
    static bool bon = false;

    // We do status updates at certain periods, and we
    // offset different operations a bit, just so not to
    // do too much all in one time slot.
    if (++_dcs_hk_cnt % DCS_STATUS_PERIOD == 0) {
        debug_printf("DCS: %d\n", _dcs_hk_cnt);
        if (_dcs_hk_cnt % 2 == 0) {
            aon = !aon;
            ledA_on(aon);
        }
        if (_dcs_hk_cnt % 3 == 0) {
            bon = !bon;
            ledB_on(bon);
        }
    }
    if (_dcs_hk_cnt % (DCS_STATUS_PERIOD + 3) == 0) {
        hid_update_sensbank(sensbank_get_chg());
    }
    if ((_dcs_hk_cnt % DCS_HOST_STATUS_PERIOD) == 0) {
        // Send our status to the host.
        //printf("DCS: %d A:%d B:%d\n", _dcs_hk_cnt, aon, bon);
    }
}

static void _handle_dcs_test(cmt_msg_t* msg) {
    // Test `scheduled_msg_ms` error
    static int times = 1;
    static cmt_msg_t msg_time = { MSG_DCS_TEST, MSG_PRI_NORM };

    uint64_t period = 60;

    bool ZZZ = false;
    if (ZZZ && debug_mode_enabled()) {
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

static void _handle_hwos_started(cmt_msg_t* msg) {
    // The Hardware Operating System has reported that it is started.
    // Since we are responding to a message, it means we
    // are also initialized, so -
    //
    // Start things running.
    _hwos_started = true;
    _dcs_started();
}

static void _handle_sensbank_chg(cmt_msg_t* msg) {
    // There was a change in the SenseBank. Update the HID.
    hid_update_sensbank(msg->data.sensbank_chg);
}

// ====================================================================
// Local functions
// ====================================================================

static void _dcs_started() {
    // Start the HID
    hid_start();

    // Let the HW level know that we are started.
    cmt_msg_t msg;
    cmt_msg_init(&msg, MSG_DCS_STARTED);
    postHWCtrlMsg(&msg);
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

    // Initialize the Human Interface Device system
    hid_module_init();
}

void start_dcs() {
    static bool _started = false;
    // Make sure we aren't already started and that we are being called from core-0.
    assert(!_started && 0 == get_core_num());
    _started = true;
    start_core1(); // The Core-1 main starts the DCS
}
