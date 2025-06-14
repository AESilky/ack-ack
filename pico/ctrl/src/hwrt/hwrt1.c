/**
 * Hardware Runtime for Board-1.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "hwrt.h"

#include "board.h"
#include "debug_support.h"

#include "cmt/cmt.h"
#include "hid/hid.h"
#include "util/util.h"

#include "pico/stdlib.h"
#include "pico/float.h"
#include "pico/printf.h"

#define _HWRT_STATUS_PULSE_PERIOD 6999

static bool _dcs_started = false;

// Message handler functions...
static void _handle_hwrt_housekeeping(cmt_msg_t* msg);
static void _handle_hwrt_test(cmt_msg_t* msg);
static void _handle_hid_started(cmt_msg_t* msg);

// ====================================================================
// Message handler functions
// ====================================================================

static void _handle_hid_started(cmt_msg_t* msg) {
    // The UI has reported that it is initialized.
    // Since we are responding to a message, it means we
    // are also initialized, so -
    //
    // Start things running.
    _dcs_started = true;
}

/**
 * @brief Handle HW Runtime Housekeeping tasks. This is triggered every ~16ms.
 *
 * For reference, 625 times is 10 seconds.
 *
 * @param msg Nothing important in the message.
 */
static void _handle_hwrt_housekeeping(cmt_msg_t* msg) {
    static int cnt = 0;

    cnt++;

}

static void _handle_hwrt_test(cmt_msg_t* msg) {
    // Test `scheduled_msg_ms` error
    static int times = 1;

    cmt_msg_t msg2;
    uint64_t period = 60;

    if (debug_mode_enabled()) {
        uint64_t now = now_us();

        uint64_t last_time = msg->data.ts_us;
        int64_t error = ((now - last_time) - (period * 1000 * 1000));
        float error_per_ms = ((error * 1.0) / (period * 1000.0));
        info_printf("\n%5.5d - Scheduled msg delay error us/ms:%5.2f\n", times, error_per_ms);
    }
    cmt_msg_init2(&msg2, MSG_HWRT_TEST, MSG_PRI_LOW);
    msg2.data.ts_us = now_us(); // Get the 'next' -> 'last_time' fresh
    schedule_msg_in_ms((period * 1000), &msg2);
    times++;
}


// ====================================================================
// Hardware operational functions
// ====================================================================


// ====================================================================
// Initialization and Startup functions
// ====================================================================

void hwrt_started() {
    // Will be called by the CMT message loop processor when the message loop is ready.
    //

    //
    // Done with the Hardware Runtime Startup - Let the HID know.
    cmt_msg_t msg;
    cmt_msg_init(&msg, MSG_HWRT_STARTED);
    postDCSMsg(&msg);
    // Post a TEST to ourself in case we have any tests set up.
    cmt_msg_init2(&msg, MSG_HWRT_TEST, MSG_PRI_LOW);
    postHWRTMsg(&msg);
}

void start_hwrt() {
    static bool _started = false;
    // Make sure we aren't already started and that we are being called from core-0.
    assert(!_started && 0 == get_core_num());
    _started = true;
    // Enter into the message loop.
    message_loop(hwrt_started);
}


void hwrt_module_init() {
    cmt_msg_hdlr_add(MSG_HOUSEKEEPING_RT, _handle_hwrt_housekeeping);
    cmt_msg_hdlr_add(MSG_HWRT_TEST, _handle_hwrt_test);
    cmt_msg_hdlr_add(MSG_HID_STARTED, _handle_hid_started);
}
