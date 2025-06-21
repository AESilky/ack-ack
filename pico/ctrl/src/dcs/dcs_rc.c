/**
 * dcs_rc - Drive Control System - Remote Control Reader-Processor.
 *
 * This gets the channel values from the RC input processing and does initial
 * control-level processing before providing status and values to the DCS.
 *
 * For switch values (Direct Control, Forward-Rotate-Reverse, ) it assures the
 * changed value remains consistent for 350ms before notifying the DCS of the
 * change. For those controls, it is quick enough for good response, while
 * keeping the DCS from changing between states too often.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "dcs_rc.h"

#include "board.h"
#include "cmt/cmt.h"
#include "rcrx/rcrx.h"

#include <math.h>

#define RC_SW_STEADY_MS 550

// Direct Control state
static bool _dc;
static bool _dc_new;        // For debouncing
// Direct Control Channel
static uint8_t _dcch;
// Forward-Rotate-Reverse state
static dcs_frr_t _frr;
static dcs_frr_t _frr_new;  // For debouncing
// Forward-Rotate-Reverse Channel
static uint8_t _frrch;
// Steering - adjusted to 0~1000 (mid=500, rolling average)
static uint16_t _steering;
// Steering Channel
static uint8_t _strch;
// Throttle - adjusted to 0-100% (from the rolling average)
static uint16_t _throttle;
// Throttle Channel
static uint8_t _thrtch;


// ====================================================================
// Local Methods
// ====================================================================

static void _post_dc_chg() {
    cmt_msg_t msg;
    cmt_msg_init(&msg, MSG_DIRECT_CTRL_CHG);
    msg.data.bv = _dc;
    postDCSMsg(&msg);
}

static void _dc_chg_delay(cmt_msg_t* msg) {
    bool dc_new = msg->data.bv;
    // Make sure the 'new' value is still different before posting the
    // 'changed' message.
    if (_dc != dc_new) {
        _dc_new = dc_new;
        _dc = dc_new;
        _post_dc_chg();
    }
}

/**
 * @brief Read the 'Direct Control' state value from the RC Channel Buffer.
 *
 * If the RC Buffer indicates that it is in 'Fail-Safe' turn 'Direct Control'
 * off.
 */
static void _rc_rd_dc_state() {
    const rcrx_state_t* chst = rcrx_get_ch_state();
    bool dc_now = false; // Set up for 'failsafe'
    bool fs = chst->failsafe;
    if (!fs) {
        dc_now = (chst->ch_data[_dcch].v > 0);
    }
    if (dc_now != _dc_new) {
        // If the state changed and the radio is in 'failsafe' post the
        // change message immediately.
        _dc_new = dc_now;
        scheduled_msg_cancel2(MSG_EXEC, _dc_chg_delay); // Cancel any pending delay
        if (fs) {
            _dc = _dc_new;
            _post_dc_chg();
        }
        else {
            // We want to wait for the value to be consistent for the
            // 'settle' time.
            cmt_msg_t msg;
            cmt_msg_init2(&msg, MSG_EXEC, _dc_chg_delay);
            msg.data.bv = dc_now;
            schedule_msg_in_ms(RC_SW_STEADY_MS, &msg);
        }
    }
}

static void _post_frr_chg() {
    cmt_msg_t msg;
    cmt_msg_init(&msg, MSG_FORWARD_ROTATE_REVERSE_CHG);
    msg.data.value16 = (int16_t)_frr;
    postDCSMsg(&msg);
}

static void _frr_chg_delay(cmt_msg_t* msg) {
    dcs_frr_t frr_new = (dcs_frr_t)msg->data.value16;
    // Make sure the 'new' value is still different before posting the
    // 'changed' message.
    if (_frr != frr_new) {
        _frr_new = frr_new;
        _frr = frr_new;
        _post_frr_chg();
    }
}

static void _rc_rd_frr_control() {
    const rcrx_state_t* chst = rcrx_get_ch_state();
    dcs_frr_t frr_now = DCS_FRR_ROTATE;  // Set up for failsafe
    bool fs = chst->failsafe;
    if (!fs) {
        int16_t v = chst->ch_data[_frrch].v;
        if (v < -500) {
            frr_now = DCS_FRR_FORWARD;
        }
        else if (v > 500) {
            frr_now = DCS_FRR_REVERSE;
        }
        else {
            frr_now = DCS_FRR_ROTATE;
        }
    }
    if (frr_now != _frr_new) {
        // If the state changed and the radio is in 'failsafe' post the
        // change message immediately.
        _frr_new = frr_now;
        scheduled_msg_cancel2(MSG_EXEC, _frr_chg_delay); // Cancel any pending delay
        if (fs) {
            _frr = frr_now;
            _post_frr_chg();
        }
        else {
            // We want to wait for the value to be consistent for the
            // 'settle' time.
            cmt_msg_t msg;
            cmt_msg_init2(&msg, MSG_EXEC, _frr_chg_delay);
            msg.data.value16 = frr_now;
            schedule_msg_in_ms(RC_SW_STEADY_MS, &msg);
        }
    }
}

/**
 * @brief Read and adjust the values of the Steering and Throttle channels.
 *
 * The values are adjusted to match what is used by the HiWonder Bus Servos,
 * such that the value can be used directly to control a servo (even if
 * that isn't actually done, it helps reduce the number of math operations
 * that need to be performed).
 *
 * Steering is adjusted to a value of 0~1000, with 500 being center.
 * Throttle is adjusted to a value of 0~900 (only forward)
 *
 */
static void _rc_rd_strthrt() {
    const rcrx_state_t* chst = rcrx_get_ch_state();
    bool fs = chst->failsafe;

    if (fs) {
        // During 'failsafe' set steering to neutral and throttle to 0.
        _steering = 500;
        _throttle = 0;
        return;
    }
    int16_t sraw = chst->ch_data[_strch].v;
    int16_t traw = chst->ch_data[_thrtch].v;
    //
    // Adjust raw values
    sraw += 10000; // Move value up to 0~20000 (from -10000~0~10000)
    sraw = ((sraw < 0) ? 0 : ((sraw > 20000) ? 20000 : sraw));
    traw += 10000; // Move value up to 0~20000 (from -10000~0~10000)
    traw = ((traw < 0) ? 0 : ((traw > 20000) ? 20000 : traw));
    //
    // Calculate the servo values
    uint16_t scal = (uint16_t)round((float)sraw * 0.05);
    uint16_t tcal = (uint16_t)round((float)traw * 0.045);
    //
    // Calculate running average
    float avg = ((float)(_steering + scal) / 2.0);
    _steering = (uint16_t)round(avg);
    avg = ((float)(_throttle + tcal) / 2.0);
    tcal = (uint16_t)round(avg);
    _throttle = ((tcal < 5) ? 0 : tcal); // Make 0~4 = 0 to avoid drift
}

// ====================================================================
// Message handler functions
// ====================================================================

/**
 * @brief Handle a Radio Control Receiver Update.
 *
 * @param msg
 */
static void _handle_rcrx_update(cmt_msg_t* msg) {
    _rc_rd_dc_state();
    _rc_rd_frr_control();
    _rc_rd_strthrt();
}

/**
 * @brief Handle a Radio Control Receiver 'FailSafe' changed.
 *
 * @param msg
 */
static void _handle_rcrx_failsafe_chg(cmt_msg_t* msg) {
    _rc_rd_dc_state();
    _rc_rd_frr_control();
    _rc_rd_strthrt();
}


// ====================================================================
// Public Methods
// ====================================================================

uint8_t dcs_rc_dcch() {
    return _dcch;
}

void dcs_rc_dcch_set(uint8_t channel) {
    _dcch = channel;
}

bool dcs_rc_direct_ctrl() {
    return _dc;
}

uint8_t dcs_rc_frrch() {
    return _frrch;
}

void dcs_rc_frrch_set(uint8_t channel) {
    _frrch = channel;
}

dcs_frr_t dcs_rc_fwd_rot_rev() {
    return _frr;
}

dcs_st_t dcs_rc_st() {
    dcs_st_t st = {_steering, _throttle};
    return (st);
}

uint8_t dcs_rc_strch() {
    return _strch;
}

void dcs_rc_strch_set(uint8_t channel) {
    _strch = channel;
}

uint16_t dcs_rc_steering() {
    return _steering;
}

uint8_t dcs_rc_thrtch() {
    return _thrtch;
}

void dcs_rc_thrtch_set(uint8_t channel) {
    _thrtch = channel;
}

uint16_t dcs_rc_throttle() {
    return _throttle;
}

// ====================================================================
// Initialization and Start-Up Methods
// ====================================================================

void dcs_rc_start() {
    cmt_msg_hdlr_add(MSG_RC_RECEIVED, _handle_rcrx_update);
    cmt_msg_hdlr_add(MSG_RC_FAILSAFE_CHG, _handle_rcrx_failsafe_chg);
}

void dcs_rc_module_init() {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("!!! `dcs_rc_module_init` called more than once !!!");
    }
    _initialized = true;

    _dc = false;
    _dcch = CH_DIRECT_CTRL_SEL;
    _frr = DCS_FRR_ROTATE;
    _frrch = CH_FWD_ROT_REV;
    _strch = CH_STEERING;
    _thrtch = CH_THROTTLE;
}
