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
#include "rover/servos.h"
#include "util/util.h"  // For 'constrain' and other macros

#include <math.h>

#define RC_SW_STEADY_MS 550

// ====================================================================
// Method Declarations
// ====================================================================

static void _dc_msg_proc(cmt_msg_t* msg);
static void _frr_msg_proc(cmt_msg_t* msg);


// ====================================================================
// Data
// ====================================================================

// Direct Control state
static bool _dc;
static bool _dc_new;            // For debouncing
static volatile bool _dc_mp;    // Direct Control MSG Pending
// Direct Control Channel
static uint8_t _dcch;
// Forward-Rotate-Reverse state
static dcs_frr_t _frr;
static dcs_frr_t _frr_new;      // For debouncing
static volatile bool _frr_mp;   // Forward-Rotate-Reverse MSG Pending
// Forward-Rotate-Reverse Channel
static uint8_t _frrch;
// Throttle - adjusted to 0-100% (from the rolling average)
static uint16_t _throttle;
// Throttle Channel
static uint8_t _thrtch;
// Yaw (Steering) - adjusted to yaw_min~500~yaw_max (mid=500, rolling average)
static uint16_t _yaw;
// Yaw (Steering)- raw (nearly) from the RC (cleaned to -10000 to 10000, rolling average)
static int16_t _yaw_raw;
// Yaw (Steering) Channel
static uint8_t _yawch;
// Yaw maximum
static uint16_t _yawmax;
// Yaw minimum
static uint16_t _yawmin;
// yaw value adjustment (from RC Channel value to yaw value)
static float _yawadj;
// yaw mid-point (used to provide a center 'null' zone)
static uint16_t _yawmid;
// yaw range (yaw max - yaw min)
static uint16_t _yawrange;

// ====================================================================
// Local Methods
// ====================================================================

static inline void _yaw_range_update() {
    _yawrange = _yawmax - _yawmin;
    _yawadj = (float)_yawrange / 20000.0;
    _yawmid = (_yawrange / 2) + _yawmin;
}

static void _post_dc_chg() {
    if (!_dc_mp) {
        _dc_mp = true;
        cmt_msg_t msg;
        cmt_msg_init2(&msg, MSG_DIRECT_CTRL_CHG, _dc_msg_proc);
        msg.data.bv = _dc;
        postDCSMsg(&msg);
    }
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
        if (!dc_now) {
            // If direct control is now off - stop the rover (ZZZ - This may change in the future)
            servos_stop();
        }
        else {
            servos_start();
        }
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
    if (!_frr_mp) {
        _frr_mp = true;
        cmt_msg_t msg;
        cmt_msg_init2(&msg, MSG_FORWARD_ROTATE_REVERSE_CHG, _frr_msg_proc);
        msg.data.value16 = (int16_t)_frr;
        postDCSMsg(&msg);
    }
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
 * @brief Read and adjust the values of the Yaw (Steering) and Throttle channels.
 *
 * The yaw/steering value is adjusted to match what is used by the HiWonder
 * Bus Servos, such that the value can be used directly to control a servo (even
 * if that isn't actually done, it helps reduce the number of math operations
 * that need to be performed).
 *
 * Yaw (Steering) is adjusted to a value of yaw_min~yaw_max, with 500 being center.
 * Throttle is 0 to 100% (only forward, the DCS or other needs to control forward
 * and reverse).
 */
static void _rc_rd_yawthrt() {
    const rcrx_state_t* chst = rcrx_get_ch_state();
    bool fs = chst->failsafe;

    if (fs) {
        // During 'failsafe' set yaw to neutral and throttle to 0.
        _yaw = 500;
        _yaw_raw = 0;
        _throttle = 0;
        return;
    }
    int16_t yraw = chst->ch_data[_yawch].v;
    int16_t traw = chst->ch_data[_thrtch].v;
    //
    // Constrain/Adjust raw values
    int16_t yadj = yraw + 10000; // Move value up to 0~20000 (from -10000~0~10000)
    yadj = constrain(yadj, 0, 20000);
    traw += 10000; // Move value up to 0~20000 (from -10000~0~10000)
    float tp = ((traw <= 0) ? 0.0f : ((traw > 20000) ? 100.0f : ((float)traw / 198.0f))); // Bump the 100% value a bit
    yraw = constrain(yraw, -10000, 10000);
    //
    // Calculate the yaw servo value
    float yc = (float)yadj * _yawadj;
    uint16_t ycal = (uint16_t)round(yc + _yawmin);
    //
    // Calculate running average
    float avg = ((float)(_yaw + ycal) / 2.0);
    ycal = (uint16_t)roundf(avg);
    _yaw = ((ycal < (_yawmid - 2)) ? ycal : (ycal > (_yawmid + 2)) ? ycal : _yawmid);
    avg = ((float)(_yaw_raw + yraw) / 2.0);
    _yaw_raw = avg;
    avg = ((float)_throttle + tp) / 2.0;
    uint16_t tcal = (uint16_t)roundf(avg);
    _throttle = ((tcal < 5) ? 0 : ((tcal > 100) ? 100 : tcal)); // Make 0~4 = 0 to avoid drift/jitter
}

// ====================================================================
// Message handler functions
// ====================================================================

/**
 * @brief Clear 'Direct Control' Pending flag on receipt of message.
 *
 * @param msg Nothing needed
 */
static void _dc_msg_proc(cmt_msg_t* msg) {
    _dc_mp = false;     // Clear the pending flag
}

/**
 * @brief Clear the 'Forward-Rotate-Reverse' Pending flag on receipt of message.
 *
 * @param msg Nothing needed
 */
static void _frr_msg_proc(cmt_msg_t* msg) {
    _frr_mp = false;    // Clear the pending flag
}

/**
 * @brief Handle a Radio Control Receiver Update.
 *
 * @param msg
 */
static void _handle_rcrx_update(cmt_msg_t* msg) {
    _rc_rd_dc_state();
    _rc_rd_frr_control();
    _rc_rd_yawthrt();
}

/**
 * @brief Handle a Radio Control Receiver 'FailSafe' changed.
 *
 * @param msg
 */
static void _handle_rcrx_failsafe_chg(cmt_msg_t* msg) {
    _rc_rd_dc_state();
    _rc_rd_frr_control();
    _rc_rd_yawthrt();
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

uint8_t dcs_rc_thrtch() {
    return _thrtch;
}

void dcs_rc_thrtch_set(uint8_t channel) {
    _thrtch = channel;
}

uint16_t dcs_rc_throttle() {
    return _throttle;
}

dcs_yt_t dcs_rc_yt() {
    dcs_yt_t st = { _yaw, _throttle };
    return (st);
}

uint16_t dcs_rc_yaw() {
    return _yaw;
}

uint8_t dcs_rc_yawch() {
    return _yawch;
}

void dcs_rc_yawch_set(uint8_t channel) {
    _yawch = channel;
}

uint16_t dcs_rc_yawmax() {
    return _yawmax;
}

void dcs_rc_yawmax_set(uint16_t value) {
    _yawmax = value;
    _yaw_range_update();
}

uint16_t dcs_rc_yawmin() {
    return _yawmin;
}

void dcs_rc_yawmin_set(uint16_t value) {
    _yawmin = value;
    _yaw_range_update();
}

int16_t dcs_rc_yaw_raw() {
    return _yaw_raw;
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
    _yawch = CH_YAW;
    _thrtch = CH_THROTTLE;
    _yawmin = 0;
    _yawmax = 1000;
    _yaw_range_update();
}
