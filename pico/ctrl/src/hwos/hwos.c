/**
 * hwctrl Back-End - Base.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "hwos.h"

#include "board.h"
#include "debug_support.h"

#include "cmt/cmt.h"
#include "curswitch/curswitch.h"
#include "display/display.h"
#include "rotary_encoder/re_pbsw.h"
#include "rotary_encoder/rotary_encoder.h"
#include "rover/rover.h"
#include "servo/servo_mh.h"
#include "servo/servos.h"
#include "util/util.h"

#include "hardware/rtc.h"

#include <stdlib.h>

#define _HWOS_STATUS_PULSE_PERIOD 6999

static switch_id_t _sw_pressed = SW_NONE;
static cmt_msg_t _sw_longpress_msg = { MSG_SW_LONGPRESS_DELAY, MSG_PRI_NORM };
static bool _input_sw_pressed;
static cmt_msg_t _input_sw_debounce_msg = { MSG_INPUT_SW_DEBOUNCE, MSG_PRI_NORM };
static bool _dcs_started = false;

// Message handler functions...
static void _handle_hwos_housekeeping(cmt_msg_t* msg);
static void _handle_hwos_test(cmt_msg_t* msg);
static void _handle_input_sw_debounce(cmt_msg_t* msg);
static void _handle_rotary_change(cmt_msg_t* msg);
static void _handle_switch_action(cmt_msg_t* msg);
static void _handle_switch_longpress_delay(cmt_msg_t* msg);
static void _handle_dcs_started(cmt_msg_t* msg);

// Idle functions...
static void _hwos_idle_function_1();
static void _hwos_idle_function_2();

// Hardware functions...
static void _input_sw_irq_handler(uint32_t events);


static const msg_handler_entry_t _hwos_housekeeping = { MSG_HOUSEKEEPING_RT, _handle_hwos_housekeeping };
static const msg_handler_entry_t _hwos_test = { MSG_HWOS_TEST, _handle_hwos_test };
static const msg_handler_entry_t _input_sw_debnce_handler_entry = { MSG_INPUT_SW_DEBOUNCE, _handle_input_sw_debounce };
static const msg_handler_entry_t _rotary_chg_handler_entry = { MSG_ROTARY_CHG, _handle_rotary_change };
static const msg_handler_entry_t _switch_action_handler_entry = { MSG_SWITCH_ACTION, _handle_switch_action };
static const msg_handler_entry_t _switch_longpress_handler_entry = { MSG_SW_LONGPRESS_DELAY, _handle_switch_longpress_delay };
static const msg_handler_entry_t _dcs_started_handler_entry = { MSG_DCS_STARTED, _handle_dcs_started };

// For performance - put these in order that we expect to receive more often
static const msg_handler_entry_t* _hwos_handler_entries[] = {
    & cmt_sm_tick_handler_entry,    // CMT Scheduled Message 'Tick'
    & _hwos_housekeeping,
    & servo_rxd_handler_entry,
    & _switch_action_handler_entry,
    & _switch_longpress_handler_entry,
    & _input_sw_debnce_handler_entry,
    & _rotary_chg_handler_entry,
    & _dcs_started_handler_entry,
    & _hwos_test,
    ((msg_handler_entry_t*)0), // Last entry must be a NULL
};

static const idle_fn _hwos_idle_functions[] = {
    // Cast needed do to definition needed to avoid circular reference.
    (idle_fn)_hwos_idle_function_1,
    (idle_fn)_hwos_idle_function_2,
    (idle_fn)0, // Last entry must be a NULL
};

msg_loop_cntx_t hwos_msg_loop_cntx = {
    HWOS_CORE_NUM, // Hardware OS runs on Core 0
    _hwos_handler_entries,
    _hwos_idle_functions,
};

// ====================================================================
// Idle functions
//
// Something to do when there are no messages to process.
// (These are cycled through, so do one task.)
// ====================================================================

/**
 * @brief Do an actual check of the input switch to make sure we stay
 *        in sync with the actual state (not just relying on the
 *        interrupts).
 * @ingroup be
 */
static void _hwos_idle_function_1() {
    _input_sw_pressed = _input_sw_pressed && user_switch_pressed();
}

/**
 * @brief Do periodic, non-critical, system updates during idle time.
 * @ingroup be
 */
static void _hwos_idle_function_2() {
}


// ====================================================================
// Message handler functions
// ====================================================================

/**
 * @brief Handle HW OS Housekeeping tasks. This is triggered every ~16ms.
 *
 * @param msg Nothing important in the message.
 */
static void _handle_hwos_housekeeping(cmt_msg_t* msg) {
    // Do cleanup, status updates, heartbeat, etc.
    if (_dcs_started) {
        curswitch_trigger_read();  // Read the switch banks
    }
    servos_housekeeping();
    rover_housekeeping();
}

static void _handle_hwos_test(cmt_msg_t* msg) {
    // Test `scheduled_msg_ms` error
    static int times = 1;
    static cmt_msg_t msg_time = { MSG_HWOS_TEST, MSG_PRI_NORM };

    uint64_t period = 60;

    if (debug_mode_enabled()) {
        uint64_t now = now_us();

        uint64_t last_time = msg->data.ts_us;
        int64_t error = ((now - last_time) - (period * 1000 * 1000));
        float error_per_ms = ((error * 1.0) / (period * 1000.0));
        info_printf(true, "\n%5.5d - Scheduled msg delay error us/ms:%5.2f\n", times, error_per_ms);
    }
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

static void _handle_rotary_change(cmt_msg_t* msg) {
    // The rotary encoder has been turned.
    int32_t rotary_cnt = re_count();
    debug_printf(false, "RE: p:%5d d:%3hd\n", rotary_cnt, msg->data.rotary_delta);
}

static void _handle_switch_action(cmt_msg_t* msg) {
    // Handle switch actions so we can detect a long press
    // and post a message for it.
    //
    // We keep track of one switch in each bank. We assume
    // that only one switch (per bank) can be pressed at
    // a time, so we only keep track of the last one pressed.
    //
    switch_id_t sw_id = msg->data.sw_action.switch_id;
    bool pressed = msg->data.sw_action.pressed;
    if (!pressed) {
        // Clear any long press in progress
        scheduled_msg_cancel(MSG_SW_LONGPRESS_DELAY);
        _sw_pressed = SW_NONE;
    }
    else {
        // Start a delay timer
        switch_action_data_t *sad = &_sw_longpress_msg.data.sw_action;
        _sw_pressed = sw_id;
        sad->switch_id = sw_id;
        sad->pressed = true;
        sad->repeat = false;
        schedule_msg_in_ms(SWITCH_LONGPRESS_MS, &_sw_longpress_msg);
    }
}

static void _handle_switch_longpress_delay(cmt_msg_t* msg) {
    // Handle the long press delay message to see if the switch is still pressed.
    switch_id_t sw_id = msg->data.sw_action.switch_id;
    bool repeat = msg->data.sw_action.repeat;
    cmt_msg_t* slpmsg = NULL;
    if (sw_id == _sw_pressed) {
        // Prepare to post another delay msg.
        slpmsg = &_sw_longpress_msg;
    }
    else {
        sw_id = SW_NONE;
    }
    if (sw_id != SW_NONE) {
        // Yes, the same switch is still pressed
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_SWITCH_LONGPRESS);
        msg.data.sw_action.switch_id = sw_id;
        msg.data.sw_action.pressed = true;
        msg.data.sw_action.repeat = repeat;
        postBothMsgDiscardable(&msg);
        // Schedule another delay
        switch_action_data_t* sad = &slpmsg->data.sw_action;
        sad->switch_id = sw_id;
        sad->pressed = true;
        sad->repeat = true;
        uint16_t delay = (repeat ? SWITCH_REPEAT_MS : SWITCH_LONGPRESS_MS);
        schedule_msg_in_ms(delay, slpmsg);
    }
}

static void _handle_dcs_started(cmt_msg_t* msg) {
    // The UI has reported that it is initialized.
    // Since we are responding to a message, it means we
    // are also initialized, so -
    //
    // Start things running.
    _dcs_started = true;
}


// ====================================================================
// Hardware operational functions
// ====================================================================


void _gpio_irq_handler(uint gpio, uint32_t events) {
    switch (gpio) {
    case IRQ_INPUT_SW:
        _input_sw_irq_handler(events);
        break;
    case IRQ_ROTARY_TURN:
        re_turn_irq_handler(gpio, events);
        break;
    }
}

static void _input_sw_irq_handler(uint32_t events) {
    // The user input switch and the infrared receiver B share the same GPIO.
    // The GPIO needs to be low for at least 80ms to be considered a button press.
    // Anything shorter is probably the IR, which is handled by PIO.
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Delay to see if it is user input or an IR received.
        // Check to see if we have already scheduled a debounce message.
        if (!scheduled_message_exists(MSG_INPUT_SW_DEBOUNCE)) {
            schedule_msg_in_ms(80, &_input_sw_debounce_msg);
        }
    }
    if (events & GPIO_IRQ_EDGE_RISE) {
        // If we haven't recorded the input switch as pressed, this is probably the IR-B
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
// Initialization functions
// ====================================================================


void hwos_module_init() {
    _input_sw_pressed = false;
    re_pbsw_module_init();
    rotary_encoder_module_init();
    gpio_set_irq_enabled_with_callback(IRQ_ROTARY_TURN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &_gpio_irq_handler);
    // gpio_set_irq_enabled(IRQ_rotary_SW, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    // gpio_set_irq_enabled(IRQ_TOUCH, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Init the rover control.
    rover_module_init();

    // Post a TEST to ourself in case we have any tests set up.
    cmt_msg_t msg;
    cmt_msg_init2(&msg, MSG_HWOS_TEST, MSG_PRI_LP);
    postHWCtrlMsgDiscardable(&msg);
}

void hwos_started() {
    // Will be called by the CMT message loop processor when the message loop is ready.
    //
    // Setup the screen with a fixed top area with enough room for 2 status lines.
    disp_print_wrap_len_set(4);
    disp_scroll_area_define(2, 0);
    disp_cursor_home();
    disp_clear(Paint);
    disp_cursor_show(true);
    //
    // Start the Rover processing.
    rover_start();
    //
    // Done with the Hardware OS Startup - Let the DSC know.
    cmt_msg_t msg;
    cmt_msg_init(&msg, MSG_HWOS_STARTED);
    postDCSMsg(&msg);
}

void start_hwos() {
    static bool _started = false;
    // Make sure we aren't already started and that we are being called from core-0.
    assert(!_started && 0 == get_core_num());
    _started = true;
    // Enter into the message loop.
    message_loop(&hwos_msg_loop_cntx, hwos_started);
}
