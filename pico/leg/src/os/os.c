/**
 * leg Operating System - Board/Hardware Ops Base.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "os.h"

#include "board.h"
#include "system_defs.h"
#include "debug_support.h"

#include "cmt/cmt.h"
#include "config/config.h"
#include "util/util.h"

#include "hardware/rtc.h"

#include <stdlib.h>

#define _OS_STATUS_PULSE_PERIOD 6999

static bool _input_sw_pressed;
static cmt_msg_t _input_sw_debounce_msg = { MSG_INPUT_SW_DEBOUNCE };
static config_t* _last_cfg;
static bool _fl_initialized = false;

// Message handler functions...
static void _handle_os_test(cmt_msg_t* msg);
static void _handle_config_changed(cmt_msg_t* msg);
static void _handle_input_sw_debounce(cmt_msg_t* msg);
static void _handle_fl_initialized(cmt_msg_t* msg);

// Idle functions...
static void _os_idle_function_1();
static void _os_idle_function_2();

// Hardware functions...
static void _input_sw_irq_handler(uint32_t events);


static const msg_handler_entry_t _os_test = { MSG_OS_TEST, _handle_os_test };
static const msg_handler_entry_t _config_changed_handler_entry = { MSG_CONFIG_CHANGED, _handle_config_changed };
static const msg_handler_entry_t _input_sw_debnce_handler_entry = { MSG_INPUT_SW_DEBOUNCE, _handle_input_sw_debounce };
static const msg_handler_entry_t _fl_initialized_handler_entry = { MSG_UI_INITIALIZED, _handle_fl_initialized };

// For performance - put these in order that we expect to receive more often
static const msg_handler_entry_t* _os_handler_entries[] = {
    & cmt_sm_tick_handler_entry,
    & _config_changed_handler_entry,
    & _input_sw_debnce_handler_entry,
    & _fl_initialized_handler_entry,
    & _os_test,
    ((msg_handler_entry_t*)0), // Last entry must be a NULL
};

static const idle_fn _os_idle_functions[] = {
    // Cast needed do to definition needed to avoid circular reference.
    (idle_fn)_os_idle_function_1,
    (idle_fn)_os_idle_function_2,
    (idle_fn)0, // Last entry must be a NULL
};

msg_loop_cntx_t os_msg_loop_cntx = {
    OS_CORE_NUM, // OS runs on Core 0
    _os_handler_entries,
    _os_idle_functions,
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
 * @ingroup os
 */
static void _os_idle_function_1() {
    bool pressed = user_switch_pressed();
    if (pressed != _input_sw_pressed) {
        // Delay to see if it is user input.
        // Check to see if we have already scheduled a debounce message.
        if (!scheduled_message_exists(MSG_INPUT_SW_DEBOUNCE)) {
            schedule_msg_in_ms(80, &_input_sw_debounce_msg);
        }
    }
}

/**
 * @brief Do periodic system updates during idle time.
 * @ingroup os
 */
static void _os_idle_function_2() {
    // uint32_t now = now_ms();
}


// ====================================================================
// Message handler functions
// ====================================================================

static void _handle_os_test(cmt_msg_t* msg) {
    // Test `scheduled_msg_ms` error
    static int times = 1;
    static cmt_msg_t msg_time = { MSG_OS_TEST };

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

static void _handle_config_changed(cmt_msg_t* msg) {
    // Update things that depend on the current configuration.
    const config_t* cfg = config_current();
    // See if things we care about have changed...

    // Hold on to the new config
    _last_cfg = config_copy(_last_cfg, cfg);
}

static void _handle_input_sw_debounce(cmt_msg_t* msg) {
    _input_sw_pressed = user_switch_pressed(); // See if it's still pressed
    if (_input_sw_pressed) {
        postFLMsgIDBlocking(MSG_INPUT_SW_PRESS, MSG_PRI_NORMAL);
    }
}

static void _handle_fl_initialized(cmt_msg_t* msg) {
    // The Functional-Level has reported that it is initialized.
    // Since we are responding to a message, it means we
    // are also initialized, so -
    //
    // Start things running.
    _fl_initialized = true;
}


// ====================================================================
// Hardware operational functions
// ====================================================================


void _gpio_irq_handler(uint gpio, uint32_t events) {
    switch (gpio) {
    case USER_INPUT_SW:
        _input_sw_irq_handler(events);
        break;
    }
}

static void _input_sw_irq_handler(uint32_t events) {
    // The user input switch and the infrared receiver B share the same GPIO.
    // The GPIO needs to be low for at least 80ms to be considered a button press.
    // Anything shorter is probably the IR, which is handled by PIO.
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
            postFLMsgIDBlocking(MSG_INPUT_SW_RELEASE, MSG_PRI_NORMAL);
        }
    }
}


// ====================================================================
// Initialization functions
// ====================================================================


void os_module_init() {
    _input_sw_pressed = user_switch_pressed();
    gpio_set_irq_enabled_with_callback(IRQ_INPUT_SW, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &_gpio_irq_handler);
    const config_t* cfg = config_current();
    _last_cfg = config_new(cfg);

    // Done with the OS Initialization - Let the Functional-Level know.
    postFLMsgIDBlocking(MSG_OS_INITIALIZED, MSG_PRI_NORMAL);
    // Post a TEST to ourself in case we have any tests set up.
    postOSMsgIDNoWait(MSG_OS_TEST);
}

void start_os() {
    static bool _started = false;
    // Make sure we aren't already started and that we are being called from core-0.
    assert(!_started && 0 == get_core_num());
    _started = true;
    message_loop(&os_msg_loop_cntx);
}
