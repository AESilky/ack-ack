/**
 * leg Functional Level - Base.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "system_defs.h"
#include "fl.h"
#include "fl_disp.h"
#include "core1_main.h"

#include "board.h"
#include "cmt/cmt.h"
#include "config/config.h"
#include "util/util.h"

#include "hardware/rtc.h"

#include <stdlib.h>
#include <string.h>

#define _UI_STATUS_PULSE_PERIOD 7001

static bool _initialized = false;

// Internal, non message handler, function declarations

// Message handler functions...
static void _handle_os_initialized(cmt_msg_t* msg);
static void _handle_config_changed(cmt_msg_t* msg);
static void _handle_input_switch_pressed(cmt_msg_t* msg);
static void _handle_input_switch_released(cmt_msg_t* msg);

static void _fl_idle_function_1();

static const msg_handler_entry_t _os_initialized_handler_entry = { MSG_OS_INITIALIZED, _handle_os_initialized };
static const msg_handler_entry_t _config_changed_handler_entry = { MSG_CONFIG_CHANGED, _handle_config_changed };
static const msg_handler_entry_t _input_sw_pressed_handler_entry = { MSG_INPUT_SW_PRESS, _handle_input_switch_pressed };
static const msg_handler_entry_t _input_sw_released_handler_entry = { MSG_INPUT_SW_RELEASE, _handle_input_switch_released };

/**
 * @brief List of handler entries.
 * @ingroup ui
 *
 * For performance, put these in the order that we expect to receive the most (most -> least).
 *
 */
static const msg_handler_entry_t* _handler_entries[] = {
    & cmt_sm_tick_handler_entry,
    &_input_sw_pressed_handler_entry,
    &_input_sw_released_handler_entry,
    &_config_changed_handler_entry,
    &_os_initialized_handler_entry,
    ((msg_handler_entry_t*)0), // Last entry must be a NULL
};

static const idle_fn _fl_idle_functions[] = {
    (idle_fn)_fl_idle_function_1,
    (idle_fn)0, // Last entry must be a NULL
};

msg_loop_cntx_t fl_msg_loop_cntx = {
    UI_CORE_NUM, // Functional-Level runs on Core 1
    _handler_entries,
    _fl_idle_functions,
};


// ============================================
// Idle functions
// ============================================

static void _fl_idle_function_1() {
    // Something to do when there are no messages to process.
}


// ============================================
// Message handler functions
// ============================================

static void _handle_config_changed(cmt_msg_t* msg) {

}

static void _handle_os_initialized(cmt_msg_t* msg) {
    // The OS has reported that it is initialized.
    // Since we are responding to a message, it means we
    // are also initialized, so -
}

/**
 * @brief Message handler for MSG_INPUT_SW_PRESS
 * @ingroup ui
 *
 * The OS has determined that the input switch has been pressed.
 *
 * @param msg Nothing in the data of this message.
 */
static void _handle_input_switch_pressed(cmt_msg_t* msg) {
    // Check that it's still pressed...
    if (user_switch_pressed()) {
        debug_printf("Input switch pressed\n");
    }
}

/**
 * @brief Message handler for MSG_INPUT_SW_RELEASE
 * @ingroup ui
 *
 * The OS has determined that the input switch has been released.
 *
 * @param msg Nothing in the data of this message.
 */
static void _handle_input_switch_released(cmt_msg_t* msg) {
    debug_printf("Input switch released\n");
}

// ============================================
// Internal functions
// ============================================


/////////////////////////////////////////////////////////////////////
// Functional Level Core Startup
/////////////////////////////////////////////////////////////////////
//

void _start_core1() {
    // Start up the Core 1 main.
    //
    // Core 1 must be started before FIFO interrupts are enabled.
    // (core1 launch uses the FIFO's, so enabling interrupts before
    // the FIFO's are used for the launch will result in unexpected behaviour.
    //
    multicore_launch_core1(core1_main);
}


// ============================================
// Public functions
// ============================================

/**
 * @brief Start the Functional-Level - The Core 1 message loop.
 */
void start_fl(void) {
    static bool _started = false;
    // Make sure we aren't already started and that we are being called from core-0.
    assert(!_started && 0 == get_core_num());
    _started = true;

    _start_core1(); // The Core-1 main starts the Functional-Level
}

bool fl_initialized() {
    return _initialized;
}

/**
 * @brief Initialize the User Interface (now that the message loop is running).
 */
void fl_module_init() {
    fl_disp_build();

    // Let the OS know that we are initialized
    _initialized = true;
    postOSMsgIDBlocking(MSG_UI_INITIALIZED, MSG_PRI_NORMAL);
}
