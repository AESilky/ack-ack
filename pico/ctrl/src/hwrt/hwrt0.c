/**
 * Hardware Runtime for Board-0.
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
#include "dcs/dcs.h"
#include "rcrx/rcrx.h"
#include "rover/rover.h"
#include "servo/servos.h"
#include "util/util.h"

#include "pico/stdlib.h"
#include "pico/float.h"
#include "pico/printf.h"

#define _HWRT_STATUS_PULSE_PERIOD 6999

static bool _dcs_started = false;

static cmt_msg_t _input_sw_debounce_msg = { MSG_INPUT_SW_DEBOUNCE, MSG_PRI_NORM };
static bool _input_sw_pressed;


// Interrupt handler functions...
static void _gpio_irq_handler(uint gpio, uint32_t events);
static void _input_sw_irq_handler(uint32_t events);

// Message handler functions...
static void _handle_hwrt_housekeeping(cmt_msg_t* msg);
static void _handle_hwrt_test(cmt_msg_t* msg);
static void _handle_dcs_started(cmt_msg_t* msg);


// ====================================================================
// Message handler functions
// ====================================================================

static void _handle_dcs_started(cmt_msg_t* msg) {
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

    if (cnt % 312 == 0) { // ~5 seconds
        rcrx_print_ch_state(true);
    }

    // Do cleanup, status updates, heartbeat, etc.
    servos_housekeeping();
    rover_housekeeping();
}

static void _handle_hwrt_test(cmt_msg_t* msg) {
    // Test `scheduled_msg_ms` error
    static int times = 1;

    cmt_msg_t msg_time = { MSG_HWRT_TEST, MSG_PRI_NORM };
    uint64_t period = 60;

    // if (debug_mode_enabled()) {
    //     uint64_t now = now_us();

    //     uint64_t last_time = msg->data.ts_us;
    //     int64_t error = ((now - last_time) - (period * 1000 * 1000));
    //     float error_per_ms = ((error * 1.0) / (period * 1000.0));
    //     info_printf("\n%5.5d - Scheduled msg delay error us/ms:%5.2f\n", times, error_per_ms);
    // }
    msg_time.data.ts_us = now_us(); // Get the 'next' -> 'last_time' fresh
    schedule_msg_in_ms((period * 1000), &msg_time);
    times++;
}

static void _handle_input_sw_debounce(cmt_msg_t* msg) {
    _input_sw_pressed = user_switch_pressed(); // See if it's still pressed
    if (_input_sw_pressed) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_INPUT_SW_PRESS);
        postDCSMsg(&msg);
    }
}


// ====================================================================
// Hardware operational functions
// ====================================================================


void _gpio_irq_handler(uint gpio, uint32_t events) {
    switch (gpio) {
    case IRQ_INPUT_SW:
        _input_sw_irq_handler(events);
        break;
    }
}

static void _input_sw_irq_handler(uint32_t events) {
    // The GPIO needs to be low for at least 80ms to be considered a button press.
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Delay to see if it is user input.
        // Check to see if we have already scheduled a debounce message.
        if (!scheduled_message_exists(MSG_INPUT_SW_DEBOUNCE)) {
            schedule_msg_in_ms(80, &_input_sw_debounce_msg);
        }
    }
    if (events & GPIO_IRQ_EDGE_RISE) {
        if (scheduled_message_exists(MSG_INPUT_SW_DEBOUNCE)) {
            scheduled_msg_cancel(MSG_INPUT_SW_DEBOUNCE);
        }
        if (_input_sw_pressed) {
            _input_sw_pressed = false;
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_INPUT_SW_RELEASE);
            postDCSMsg(&msg);
        }
    }
}


// ====================================================================
// Initialization and Startup functions
// ====================================================================

void hwrt_started() {
    // Will be called by the CMT message loop processor when the message loop is ready.
    //

    // Remote Control
    rcrx_start();
    //
    // Start the Rover processing.
    rover_start();
    //
    // Done with the Hardware Runtime Startup - Let the DSC know.
    cmt_msg_t msg;
    cmt_msg_init(&msg, MSG_HWRT_STARTED);
    postDCSMsg(&msg);
}

void start_hwrt() {
    static bool _started = false;
    // Make sure we aren't already started and that we are being called from core-0.
    assert(!_started && 0 == get_core_num());
    _started = true;

    // Launch the Drive Control System (core-1 Message Dispatching Loop)
    //  This also starts other 'core-1' functionality.
    start_dcs();

    // Enter into the message loop.
    message_loop(hwrt_started);
}


void hwrt_module_init() {
    cmt_msg_hdlr_add(MSG_HOUSEKEEPING_RT, _handle_hwrt_housekeeping);
    cmt_msg_hdlr_add(MSG_HWRT_TEST, _handle_hwrt_test);
    cmt_msg_hdlr_add(MSG_DCS_STARTED, _handle_dcs_started);


    _input_sw_pressed = false;

    // Set up the Drive Control System
    dcs_module_init();

    // Init the rover control functionality.
    rover_module_init();

    // Remote control
    rcrx_module_init();

    // Post a TEST to ourself in case we have any tests set up.
    cmt_msg_t msg;
    cmt_msg_init2(&msg, MSG_HWRT_TEST, MSG_PRI_LOW);
    postHWRTMsgDiscardable(&msg);
}
