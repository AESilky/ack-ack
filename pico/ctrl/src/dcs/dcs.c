/**
 * dcs - Drive Control System - Base.
 *
 * Setup for the message loop and idle processing.
 * Process driving operations.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "dcs.h"
#include "dcs_rc.h"

#include "board.h"
#include "debug_support.h"

#include "cmt/cmt.h"
#include "rcrx/rcrx.h"
#include "rover/rover.h"
#include "rover/servos.h"
#include "util/util.h"

#include "lib/json-maker/json-maker.h"

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include <stdio.h>

// Period values for periodic housekeeping/updates.
// Housekeeping message is every 16ms, so a divisor of 625 is 10 seconds
//
#define DCS_STATUS_PERIOD 313       // Every 5 seconds (313 * 0.016 = 5.008)
#define DCS_HOST_STATUS_PERIOD 938  // Send status to host every 15 seconds
#define RC_CH_RD_PERIOD 3           // Read RC Channels and act on them every 48ms
#define RC_CH_STATUS_PERIOD 126     // Print the RC Channel values (~2 seconds for now)

static bool _hwrt_started = false;

static int _dcs_hk_cnt;

/** Last Yaw and Throttle values from radio (sent to the servos). */
static dcs_yt_t _yt_srvo_last = { .yaw = 0, .throttle = 0 };
/** Last Forward-Rotate-Reverse value from the RC. */
static dcs_frr_t _frr;

// Message handler functions...
static void _handle_dcs_housekeeping(cmt_msg_t* msg);
static void _handle_dcs_test(cmt_msg_t* msg);
static void _handle_hwrt_started(cmt_msg_t* msg);

// Hardware functions...

// Internal functions


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
    _dcs_hk_cnt++;
    if (_dcs_hk_cnt % DCS_STATUS_PERIOD == 0) {
        debug_printf("DCS: %d\n", _dcs_hk_cnt);
    }
    if ((_dcs_hk_cnt % DCS_HOST_STATUS_PERIOD) == 0) {
        // Send our status to the host.
        //printf("DCS: %d A:%d B:%d\n", _dcs_hk_cnt, aon, bon);
    }
    if (_dcs_hk_cnt % RC_CH_RD_PERIOD == 0) {
        // Get the yaw and throttle and send it to the servos if in 'direct control'
        dcs_yt_t st = dcs_rc_yt();
        if (dcs_rc_direct_ctrl()) {
            if (_frr != DCS_FRR_ROTATE) {
                if (st.yaw != _yt_srvo_last.yaw) {
                    _yt_srvo_last.yaw = st.yaw;
                    _yt_srvo_last.throttle = st.throttle;
                    int16_t tv = ((_frr == DCS_FRR_FORWARD) ? st.throttle : (0 - st.throttle));
                    servos_yaw_set(st.yaw, tv);
                }
                if (st.throttle != _yt_srvo_last.throttle) {
                    _yt_srvo_last.throttle = st.throttle;
                    int16_t tv = ((_frr == DCS_FRR_FORWARD) ? st.throttle : (0 - st.throttle));
                    servos_velocity_set(tv);
                }
            }
            else {
                if (st.yaw != _yt_srvo_last.yaw) {
                    if (!servos_rip()) {
                        servos_rip_position();
                    }
                    // The radio rudder value, not the throttle, is used to control the RIP speed.
                    int16_t rsp = dcs_rc_yaw_raw();
                    servos_rip_speed(rsp);
                    _yt_srvo_last = st;
                }
            }
        }
        if (_dcs_hk_cnt % RC_CH_STATUS_PERIOD == 0) {
            printf("\nSteering: %hu   Throttle: %hu\n\n", st.yaw, st.throttle);
            rxprotocol_t rx = rcrx_get_protocol();
            if (rx == RXP_UNKNOWN) {
                printf("RC not determined (is radio on?)\n");
            }
            else {
                rcrx_print_ch_state(true);
            }
        }
    }

    // Do cleanup, status updates, heartbeat, etc.
    rover_housekeeping();
}

static void _handle_dcs_test(cmt_msg_t* msg) {
    // Test `scheduled_msg_ms` error
    static int times = 1;

    cmt_msg_t msg_time = { MSG_DCS_TEST };
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

static void _handle_direct_ctrl_chg(cmt_msg_t* msg) {
    // The 'Direct Control' state has changed. The state
    // is in the 'bool value' of the message.
    bool dc_new = msg->data.bv;

    info_printf("\nDirect Control state: %s\n", (dc_new ? "ON" : "OFF"));
}

static void _handle_frr_chg(cmt_msg_t* msg) {
    // The 'Forward-Rotate-Reverse' control has changed. The new
    // value is in the 'value16' of the message.
    _frr = (dcs_frr_t)msg->data.value16;
    _yt_srvo_last.throttle = 0; // Set the 'last' throttle to 0 to pick up change in control.
    char* s;
    switch(_frr) {
        case DCS_FRR_FORWARD:
            s = "FORWARD";
            break;
        case DCS_FRR_REVERSE:
            s = "REVERSE";
            break;
        case DCS_FRR_ROTATE:
            s = "ROTATE";
            break;
    }
    info_printf("\nForward-Rotate-Reverse control: %s\n", s);
}

static void _handle_hwrt_started(cmt_msg_t* msg) {
    // The Hardware Operating System has reported that it is started.
    _hwrt_started = true;
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
        board_panic("!!! `_dcs_started` - Called more than once. !!!");
    }

    cmt_msg_hdlr_add(MSG_HOUSEKEEPING_RT, _handle_dcs_housekeeping);
    cmt_msg_hdlr_add(MSG_DCS_TEST, _handle_dcs_test);
    cmt_msg_hdlr_add(MSG_DIRECT_CTRL_CHG, _handle_direct_ctrl_chg);
    cmt_msg_hdlr_add(MSG_FORWARD_ROTATE_REVERSE_CHG, _handle_frr_chg);
    dcs_rc_start();

    //
    // Start the Rover processing.
    rover_start();

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


static void _dcs_module_init() {
    _dcs_hk_cnt = 0;

    dcs_rc_module_init();

    // Init the rover control functionality.
    rover_module_init();

    // Once the rover module in initialized, we can get the yaw and throttle limits
    // and set them in our RC module.
    pair_uint16_t limits = servos_yaw_limits();
    dcs_rc_yawmin_set(limits.a);
    dcs_rc_yawmax_set(limits.b);
}

void start_dcs() {
    static bool _started = false;
    // Make sure we aren't already started and that we are being called from core-1.
    if(_started || 1 != get_core_num()) {
        board_panic("!!! `start_dcs` called more than once, or on the wrong core. Corenum: %hhu !!!", get_core_num());
    }
    _started = true;
    // Register our handler for the Hardware Control Runtime started.
    cmt_msg_hdlr_add(MSG_HWRT_STARTED, _handle_hwrt_started);

    _dcs_module_init();
    _dcs_started();
}
