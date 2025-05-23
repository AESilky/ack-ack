/**
 * hwctrl Back-End - Base.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "hwos.h"

#include "board.h"
#include "debug_support.h"

#include "cmt/cmt.h"
#include "curswitch/curswitch.h"
#include "display/display.h"                        // For character/line based operations
#include "display/display_rgb18/display_rgb18.h"    // For pixel/graphics based operations
#include "rotary_encoder/re_pbsw.h"
#include "rotary_encoder/rotary_encoder.h"
#include "rover/rover.h"
#include "sensbank/sensbank.h"
#include "servo/servos.h"
#include "term/term.h"
#include "touch_panel/touch.h"
#include "util/util.h"

#include "pico/stdlib.h"
#include "pico/float.h"
#include "pico/printf.h"

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
static void _handle_sensbank_change(cmt_msg_t* msg);
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
static const msg_handler_entry_t _input_sw_debounce_handler_entry = { MSG_INPUT_SW_DEBOUNCE, _handle_input_sw_debounce };
static const msg_handler_entry_t _rotary_chg_handler_entry = { MSG_ROTARY_CHG, _handle_rotary_change };
static const msg_handler_entry_t _sensbank_chg_handler_entry = { MSG_SENSBANK_CHG, _handle_sensbank_change };
static const msg_handler_entry_t _switch_action_handler_entry = { MSG_SWITCH_ACTION, _handle_switch_action };
static const msg_handler_entry_t _switch_longpress_handler_entry = { MSG_SW_LONGPRESS_DELAY, _handle_switch_longpress_delay };
static const msg_handler_entry_t _dcs_started_handler_entry = { MSG_DCS_STARTED, _handle_dcs_started };

// Include Message Handlers from other Modules
//
#include "cmt/cmt_mh.h"
#include "servo/servo_mh.h"
#include "term/term_mh.h"

// For performance - put these in order that we expect to receive more often
static const msg_handler_entry_t* _hwos_handler_entries[] = {
    & _hwos_housekeeping,
    & cmt_sm_sleep_handler_entry,    // CMT Scheduled Message 'Sleep' handler
    & servo_rxd_handler_entry,
    & _sensbank_chg_handler_entry,
    & _switch_action_handler_entry,
    & _switch_longpress_handler_entry,
    & _input_sw_debounce_handler_entry,
    & term_switch_action_handler_entry,
    & term_touch_handler_entry,
    & _rotary_chg_handler_entry,
    & _dcs_started_handler_entry,
    & _hwos_test,
    ((msg_handler_entry_t*)0), // Last entry must be a NULL
};

msg_loop_cntx_t hwos_msg_loop_cntx = {
    HWOS_CORE_NUM, // Hardware OS runs on Core 0
    _hwos_handler_entries,
};

// ====================================================================
// Message handler functions
// ====================================================================

/**
 * @brief Handle HW OS Housekeeping tasks. This is triggered every ~16ms.
 *
 * @param msg Nothing important in the message.
 */
static void _handle_hwos_housekeeping(cmt_msg_t* msg) {
    static gfx_point last_touch = {0,0};
    // static int cnt = 0;

    // if (++cnt % 625 == 0) {
    //     // Every 10 seconds, print the count
    //     char buf[32];
    //     int s = float2int(int2float(cnt) / 62.5f);
    //     snprintf(buf, 32, "Secs:%d", s);
    //     disp_string_color(0, 0, buf, C16_LT_BLUE, C16_BLACK, Paint);
    // }
    // Do cleanup, status updates, heartbeat, etc.
    if (_dcs_started) {
        curswitch_trigger_read();  // Read the switch banks
    }
    servos_housekeeping();
    rover_housekeeping();
    // Read the touch panel
    const gfx_point* dp = tp_check_display_point();
    if (dp) {
        if (dp->x != last_touch.x || dp->y != last_touch.y) {
            // Store the values and print that we were touched.
            last_touch.x = dp->x;
            last_touch.y = dp->y;
            //
            // Post a message with the touch
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_TOUCH_PANEL);
            postHWCtrlMsgDiscardable(&msg);
            // const gfx_point* pp = tp_last_panel_point();
            // const gfx_rect* bounds = tp_bounds_observed();
            // scr_position_t sp = disp_lc_from_point(dp);
            // char buf[64];
            // snprintf(buf, 63, "T Dx=%3d Dy=%3d Px=%4d Py=%4d", last_touch.x, last_touch.y, pp->x, pp->y);
            // disp_string_color(0, 0, buf, C16_LT_BLUE, C16_BLACK, No_Paint);
            // snprintf(buf, 63, "B: (%4d,%4d , %4d,%4d)", bounds->p1.x, bounds->p1.y, bounds->p2.x, bounds->p2.y);
            // disp_string_color(1, 0, buf, C16_LT_BLUE, C16_BLACK, Paint);
            // snprintf(buf, 63, "SCR: Line:%2d Col:%2d", sp.line, sp.column);
            // disp_string_color(2, 0, buf, C16_LT_BLUE, C16_BLACK, Paint);
        }
    }
}

static void _handle_hwos_test(cmt_msg_t* msg) {
    // Test `scheduled_msg_ms` error
    static int times = 1;
    static cmt_msg_t msg_time = { MSG_HWOS_TEST, MSG_PRI_NORM };

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

static void _handle_rotary_change(cmt_msg_t* msg) {
    // The rotary encoder has been turned.
    int32_t rotary_cnt = re_count();
    debug_printf("RE: p:%5d d:%3hd\n", rotary_cnt, msg->data.rotary_delta);
}

static void _handle_sensbank_change(cmt_msg_t* msg) {
    // Handle changes in the sensor bank bits.
    debug_printf("SB Chg: %02x -> %02x\n", msg->data.sensbank_chg.prev_bits, msg->data.sensbank_chg.bits);
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
    // The GPIO needs to be low for at least 80ms to be considered a button press.
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
// Initialization and Startup functions
// ====================================================================

void hwos_started() {
    // Will be called by the CMT message loop processor when the message loop is ready.
    //

    // Touch Panel initialization
    tp_module_init(5, gfxd_screen_width(), false, gfxd_screen_height(), true, 121, 2520, 122, 2603);
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


void hwos_module_init() {
    _input_sw_pressed = false;
    re_pbsw_module_init();
    rotary_encoder_module_init();
    gpio_set_irq_enabled_with_callback(IRQ_ROTARY_TURN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &_gpio_irq_handler);
    // gpio_set_irq_enabled(IRQ_rotary_SW, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    // gpio_set_irq_enabled(IRQ_TOUCH, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Init the rover control functionality.
    rover_module_init();

    // Post a TEST to ourself in case we have any tests set up.
    cmt_msg_t msg;
    cmt_msg_init2(&msg, MSG_HWOS_TEST, MSG_PRI_LP);
    postHWCtrlMsgDiscardable(&msg);
}
