/**
 * @brief Serial Bus Servo group control.
 * @ingroup servo
 *
 * Controls a group of HiWonder Serial Bus servos.
 * The servos are for the rover and are as follows:
 *
 * LF_DIR                              RF_DIR
 * LF_DRIVE                            RF_DRIVE
 *
 *                BOGIE_PIVOT
 * LM_DRIVE                            RM_DRIVE
 *
 *
 * LR_DRIVE                            RR_DRIVE
 * LR_DIR                              RR_DIR
 *
 * LF_DIR   : Turns the Left-Front drive wheel from -120° to 0 to +120°
 * LF_DRIVE : Drives the Left-Front drive wheel with a speed from -1000 to 0 to +1000
 * LM_DRIVE : Drives the Left-Middle drive wheel
 * LR_DRIVE : Drives the Left-Rear drive wheel
 * LR_DIR   : Turns the Left-Rear drive wheel
 * RF_DIR   : Turns the Right-Front drive wheel from -120° to 0 to +120°
 * RF_DRIVE : Drives the Right-Front drive wheel with a speed from -1000 to 0 to +1000
 * RM_DRIVE : Drives the Right-Middle drive wheel
 * RR_DRIVE : Drives the Right-Rear drive wheel
 * RR_DIR   : Turns the Right-Rear drive wheel
 * BOGIE_PIVOT : Reads, and can drive, the bogie pivot arm angle
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */

#include "servos.h"

#include "board.h"
#include "cmt/cmt.h"
#include "rover/rover.h"
#include "rover/rover_info.h"
#include "servo/servo.h"
#include "util/util.h"  // For 'constrain' and other macros

#include "pico/stdlib.h"

#include <math.h>


// ############################################################################
// Constants, Enumerations and Structures
// ############################################################################
//
#define DIRECTIONAL_SERVO_POS_CENTER 500    // The Bus Servos control is 0 to 1000
#define DIRECTIONAL_SERVO_POS_MAX 875       // +90° = 500 (center) + (90 / 0.24)
#define DIRECTIONAL_SERVO_POS_MIN 125       // -90° = 500 (center) - (90 / 0.24)
#define DIRECTIONAL_SERVO_MOVE_TIME 48      // Time to position the servo

#define LOGICAL_YAW_SERVO_POS_MAX 700       // -48.1° This generates 90° on the right side
#define LOGICAL_YAW_SERVO_POS_MIN 300       // +48.1° This generates 90° on the left side

typedef enum DIRECTIONAL_SERVOS_ID_ {
    SRVDIR_LF = 0,
    SRVDIR_RF = 1,
    SRVDIR_LR = 2,
    SRVDIR_RR = 3
} dir_servo_id_t;
#define DIRECTIONAL_SERVO_CNT 4

typedef enum DRIVE_SERVOS_ID_ {
    SRVDRV_LF = 0,
    SRVDRV_RF = 1,
    SRVDRV_LR = 2,
    SRVDRV_RR = 3,
    SRVDRV_LM = 4,
    SRVDRV_RM = 5
} drv_servo_id_t;
#define DRIVE_SERVO_CNT 6


// ############################################################################
// Data
// ############################################################################
//
/** Array of Directional servo IDs for use in the multi-servo calls */
static uint8_t _dir_servo_ids[] = { SRVO_ID_DIR_LF, SRVO_ID_DIR_RF, SRVO_ID_DIR_LR, SRVO_ID_DIR_RR };
static uint16_t _dir_positions[DIRECTIONAL_SERVO_CNT];
//
/** Array of Drive servos for use in the multi-servo calls */
static uint8_t _drv_servo_ids[] = { SRVO_ID_DRV_LF, SRVO_ID_DRV_RF, SRVO_ID_DRV_LR, SRVO_ID_DRV_RR, SRVO_ID_DRV_LM, SRVO_ID_DRV_RM };
/** Array of flags indicating if the servo is mounted in reverse (and therefore needs to run the 'other way') */
static bool _drv_is_reverse[DRIVE_SERVO_CNT] = { false, true, false, true, false, true };
/** Array of the calculated/specified drive speeds for each of the drive servos */
static int16_t _drv_speeds[DRIVE_SERVO_CNT];

/** @brief Flag indicating that the rover wheels are positioned for Rotate-In-Place */
static bool _rip;
/** @brief The speed factor for the center/middle wheels for Rotate-In-Place */
static float _rip_spdfctr_c;
/** @brief Left-Front and Right-Rear position for Rotate-In-Place */
static uint16_t _rip_lfrr_pos;
/** @brief Right-Front and Left-Rear position for Rotate-In-Place */
static uint16_t _rip_rflr_pos;
/** @brief The logical yaw value (in servo position units) */
static uint16_t _yaw;
/** @brief Speed Factor of the Center-Left drive wheel */
static float _spdfctr_cl;
/** @brief Speed Factor of the Center-Right drive wheel */
static float _spdfctr_cr;
/** @brief Speed Factor of the Steered-Left drive wheel */
static float _spdfctr_sl;
/** @brief Speed Factor of the Steered-Left drive wheel */
static float _spdfctr_sr;
/** @brief Last velocity used to set the drive wheel speeds */
static int16_t _velo_last;


// ############################################################################
// Function Declarations
// ############################################################################
//
static void _position_lf_mh(cmt_msg_t* msg);
static bool _position_lf(uint16_t pos, uint16_t time);
static void _position_lr_mh(cmt_msg_t* msg);
static bool _position_lr(uint16_t pos, uint16_t time);
static void _position_rf_mh(cmt_msg_t* msg);
static bool _position_rf(uint16_t pos, uint16_t time);
static void _position_rr_mh(cmt_msg_t* msg);
static bool _position_rr(uint16_t pos, uint16_t time);


// ############################################################################
// Message Handlers
// ############################################################################
//
static void _position_lf_mh(cmt_msg_t* msg) {
    // Try positioning the left-front again.
    _position_lf(msg->data.servo_params.pos, msg->data.servo_params.time);
}

static void _position_lr_mh(cmt_msg_t* msg) {
    // Try positioning the left-rear again.
    _position_lr(msg->data.servo_params.pos, msg->data.servo_params.time);
}

static void _position_rf_mh(cmt_msg_t* msg) {
    // Try positioning the right-front again.
    _position_rf(msg->data.servo_params.pos, msg->data.servo_params.time);
}

static void _position_rr_mh(cmt_msg_t* msg) {
    // Try positioning the right-rear again.
    _position_rr(msg->data.servo_params.pos, msg->data.servo_params.time);
}


// ############################################################################
// Internal Functions
// ############################################################################
//

/**
 * @brief Calculate the speeds of each of the drive wheels from the 'virtual model'
 * center-wheel speed.
 *
 * Note1: The speed factors are calculated in the yaw function or the Rotate-In-Place,
 * as changes in yaw effect the speeds needed at each of the wheels.
 *
 * @param velo Speed as percentage value from 0 to 100 (negative means reverse)
 */
static void _calc_drive_speeds(int16_t velo) {
    // Top servo speed is 1000 (or -1000), so multiply velo by 10 to get a value
    // to use with the speed-factor.
    int16_t srvospd = velo * 10;
    int16_t lcds = (int16_t)(srvospd * _spdfctr_cl);
    int16_t rcds = (int16_t)(srvospd * _spdfctr_cr);
    int16_t lsds = (int16_t)(srvospd * _spdfctr_sl);
    int16_t rsds = (int16_t)(srvospd * _spdfctr_sr);
    _drv_speeds[SRVDRV_LF] = lsds * (_drv_is_reverse[SRVDRV_LF] ? -1 : 1);
    _drv_speeds[SRVDRV_RF] = rsds * (_drv_is_reverse[SRVDRV_RF] ? -1 : 1);
    _drv_speeds[SRVDRV_LR] = lsds * (_drv_is_reverse[SRVDRV_LR] ? -1 : 1);
    _drv_speeds[SRVDRV_RR] = rsds * (_drv_is_reverse[SRVDRV_RR] ? -1 : 1);
    _drv_speeds[SRVDRV_LM] = lcds * (_drv_is_reverse[SRVDRV_LM] ? -1 : 1);
    _drv_speeds[SRVDRV_RM] = rcds * (_drv_is_reverse[SRVDRV_RM] ? -1 : 1);
}

/**
 * @brief Dedicated function to control the Left-Front servo.
 *
 * The reason for this is to handle the case when the servo can't immediately
 * be controlled (for example, if a servo read is in process). In that case,
 * this posts a message to try the operation again. This keeps happening until
 * the control is successful.
 *
 * @param pos Position to move to
 * @param time The time to take to move it
 * @return true The operation was performed
 * @return false The operation couldn't be performed (will keep trying)
 */
static bool _position_lf(uint16_t pos, uint16_t time) {
    if (servo_move(SRVO_ID_DIR_LF, pos, time)) {
        return true;
    }
    // Command couldn't be sent, post ourself a message to try again.
    cmt_msg_t msg;
    msg.data.servo_params.pos = pos;
    msg.data.servo_params.time = time;
    cmt_msg_init2(&msg, MSG_EXEC, _position_lf_mh);
    postHWRTMsg(&msg);
    return false;
}

/**
 * @brief Dedicated function to control the Left-Rear servo.
 *
 * The reason for this is to handle the case when the servo can't immediately
 * be controlled (for example, if a servo read is in process). In that case,
 * this posts a message to try the operation again. This keeps happening until
 * the control is successful.
 *
 * @param pos Position to move to
 * @param time The time to take to move it
 * @return true The operation was performed
 * @return false The operation couldn't be performed (will keep trying)
 */
static bool _position_lr(uint16_t pos, uint16_t time) {
    if (servo_move(SRVO_ID_DIR_LR, pos, time)) {
        return true;
    }
    // Command couldn't be sent, post ourself a message to try again.
    cmt_msg_t msg;
    msg.data.servo_params.pos = pos;
    msg.data.servo_params.time = time;
    cmt_msg_init2(&msg, MSG_EXEC, _position_lr_mh);
    postHWRTMsg(&msg);
    return false;
}

/**
 * @brief Dedicated function to control the Right-Front servo.
 *
 * The reason for this is to handle the case when the servo can't immediately
 * be controlled (for example, if a servo read is in process). In that case,
 * this posts a message to try the operation again. This keeps happening until
 * the control is successful.
 *
 * @param pos Position to move to
 * @param time The time to take to move it
 * @return true The operation was performed
 * @return false The operation couldn't be performed (will keep trying)
 */
static bool _position_rf(uint16_t pos, uint16_t time) {
    if (servo_move(SRVO_ID_DIR_RF, pos, time)) {
        return true;
    }
    // Command couldn't be sent, post ourself a message to try again.
    cmt_msg_t msg;
    msg.data.servo_params.pos = pos;
    msg.data.servo_params.time = time;
    cmt_msg_init2(&msg, MSG_EXEC, _position_rf_mh);
    postHWRTMsg(&msg);
    return false;
}

/**
 * @brief Dedicated function to control the Right-Rear servo.
 *
 * The reason for this is to handle the case when the servo can't immediately
 * be controlled (for example, if a servo read is in process). In that case,
 * this posts a message to try the operation again. This keeps happening until
 * the control is successful.
 *
 * @param pos Position to move to
 * @param time The time to take to move it
 * @return true The operation was performed
 * @return false The operation couldn't be performed (will keep trying)
 */
static bool _position_rr(uint16_t pos, uint16_t time) {
    if (servo_move(SRVO_ID_DIR_RR, pos, time)) {
        return true;
    }
    // Command couldn't be sent, post ourself a message to try again.
    cmt_msg_t msg;
    msg.data.servo_params.pos = pos;
    msg.data.servo_params.time = time;
    cmt_msg_init2(&msg, MSG_EXEC, _position_rr_mh);
    postHWRTMsg(&msg);
    return false;
}

// ############################################################################
// Public Functions
// ############################################################################
//

bool servos_rip() {
    return _rip;
}

void servos_rip_position() {
    // For Rotate-In-Place set the logical yaw to center (it's not used for RIP)
    _yaw = DIRECTIONAL_SERVO_POS_CENTER;
    _dir_positions[0] = _rip_lfrr_pos;
    _dir_positions[1] = _rip_rflr_pos;
    _dir_positions[2] = _rip_rflr_pos;
    _dir_positions[3] = _rip_lfrr_pos;
    servo_move_group(_dir_servo_ids, _dir_positions, 1000, DIRECTIONAL_SERVO_CNT);
    _rip = true;
}

void servos_rip_speed(int16_t rsp) {
    // Top servo speed is 1000 (or -1000), so divide rsp by 10 to get a value
    // to use for the fastest wheels.
    if (_rip) {
        int16_t srvospd = (rsp / 10);
        // Check for and eliminate creep
        if (-8 < srvospd && srvospd < 8) {
            srvospd = 0;
        }
        int16_t srvospd_c = (int16_t)roundf((float)srvospd * _rip_spdfctr_c);
        // For rsp > 0 (clockwise): Right Side is Reverse, Left Side is Forward.
        // For rsp < 0 (anti-clockwise): Right Side is Forward, Left Side is Reverse.
        //  However, the right servers are mounted in the reverse direction, so the
        //  speed value can be used as-is and the wheels will rotate correctly.
        _drv_speeds[SRVDRV_LF] = srvospd;
        _drv_speeds[SRVDRV_RF] = srvospd;
        _drv_speeds[SRVDRV_LR] = srvospd;
        _drv_speeds[SRVDRV_RR] = srvospd;
        _drv_speeds[SRVDRV_LM] = srvospd_c;
        _drv_speeds[SRVDRV_RM] = srvospd_c;
        servo_run_group(_drv_servo_ids, _drv_speeds, DRIVE_SERVO_CNT);
    }
}


void servos_velocity_set(int16_t velo) {
    if (!_rip) {
        _calc_drive_speeds(velo);
        _velo_last = velo;
        servo_run_group(_drv_servo_ids, _drv_speeds, DRIVE_SERVO_CNT);
    }
}

uint16_t servos_yaw() {
    return _yaw;
}

pair_uint16_t servos_yaw_limits() {
    pair_uint16_t p = { LOGICAL_YAW_SERVO_POS_MIN, LOGICAL_YAW_SERVO_POS_MAX };
    return p;
}

void servos_yaw_set(uint16_t yaw, int16_t velo) {
    _rip = false;
    _yaw = constrain(yaw, LOGICAL_YAW_SERVO_POS_MIN, LOGICAL_YAW_SERVO_POS_MAX);
    int y = DIRECTIONAL_SERVO_POS_CENTER - _yaw;
    float sgn = ((y < 0) ? -1.0f : 1.0f);
    float theta = fabsf(y * servo_rads_per_unit);
    // Calculate the values for each of the servos.
    //   The variable names are from the 'Wheels Model Geometry'
    float l = rover_cp_l();
    float t = rover_cp_t();
    uint16_t lf = DIRECTIONAL_SERVO_POS_CENTER;
    uint16_t lr = DIRECTIONAL_SERVO_POS_CENTER;
    uint16_t rf = DIRECTIONAL_SERVO_POS_CENTER;
    uint16_t rr = DIRECTIONAL_SERVO_POS_CENTER;
    if (theta != 0) {
        float rC = l / tanf(theta);
        // Calculate the other radii to be used for the velocity...
        float rCL, rCR, rSL, rSR; // Center-Left, Center-Right, Steered-Left, Steered-Right
        if (y <= 0) {
            rCL = rC + t;
            rCR = rC - t;
        }
        else {
            rCL = rC - t;
            rCR = rC + t;
        }
        // Calculate the steering values
        float theta_lf = atan2f(l, (rC - (t * sgn)));
        double theta_rf = atan2f(l, (rC + (t * sgn)));
        int16_t lsu = roundf((theta_lf / servo_rads_per_unit) * sgn);
        int16_t rsu = roundf((theta_rf / servo_rads_per_unit) * sgn);
        lf -= lsu;
        lr += lsu;
        rf -= rsu;
        rr += rsu;
        // Speed factor is the speed of the wheel at a radius compared to the speed
        // of the 'virtual (Wheels Model Geometry)' center wheel at its radius (to the ICR).
        // The largest radius gets a factor of 1.0 and the rest are adjusted down
        // proportionally. The corners will always be larger (or equal) to the centers.
        float l_sqrd = l * l;
        rSL = sqrtf((rCL * rCL) + l_sqrd);
        rSR = sqrtf((rCR * rCR) + l_sqrd);
        float rLG = rSL >= rSR ? rSL : rSR;
        _spdfctr_cl = rCL / rLG;
        _spdfctr_cr = rCR / rLG;
        _spdfctr_sl = rSL / rLG;
        _spdfctr_sr = rSR / rLG;
    }
    else {
        _spdfctr_cl = 1;
        _spdfctr_cr = 1;
        _spdfctr_sl = 1;
        _spdfctr_sr = 1;
    }
    _calc_drive_speeds(velo);
    _velo_last = velo;
    _dir_positions[SRVDIR_LF] = lf;
    _dir_positions[SRVDIR_RF] = rf;
    _dir_positions[SRVDIR_LR] = lr;
    _dir_positions[SRVDIR_RR] = rr;
    servo_move_group(_dir_servo_ids, _dir_positions, DIRECTIONAL_SERVO_MOVE_TIME, DIRECTIONAL_SERVO_CNT);
    servo_run_group(_drv_servo_ids, _drv_speeds, DRIVE_SERVO_CNT);
}

void servos_zero_position(int16_t time) {
    _yaw = DIRECTIONAL_SERVO_POS_CENTER;
    _dir_positions[0] = DIRECTIONAL_SERVO_POS_CENTER;
    _dir_positions[1] = DIRECTIONAL_SERVO_POS_CENTER;
    _dir_positions[2] = DIRECTIONAL_SERVO_POS_CENTER;
    _dir_positions[3] = DIRECTIONAL_SERVO_POS_CENTER;
    _spdfctr_cl = 1;
    _spdfctr_cr = 1;
    _spdfctr_sl = 1;
    _spdfctr_sr = 1;
    _calc_drive_speeds(_velo_last);
    if (time >= 0) {
        servo_run_group(_drv_servo_ids, _drv_speeds, DRIVE_SERVO_CNT);
        servo_move_group(_dir_servo_ids, _dir_positions, (uint16_t)time, DIRECTIONAL_SERVO_CNT);
    }
}


// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//

void servos_housekeeping(void) {
}

void servos_start(void) {
    rover_aux_pwr_on(true);
    servo_module_start();
    // Set all of the servos and then power them up.
    // For the directional (positional) servos, set their mode and limits.
    for (int i = 0; i < DIRECTIONAL_SERVO_CNT; i++) {
        uint8_t id = _dir_servo_ids[i];
        servo_set_mode(id, BS_POSITION_MODE, 0);  // Mode to 'Position' (speed isn't used)
        servo_set_limits(id, DIRECTIONAL_SERVO_POS_MIN, DIRECTIONAL_SERVO_POS_MAX);
    }
    // Initialize all of the drive servos to DRIVE mode at 0 speed.
    for (int i = 0; i < DRIVE_SERVO_CNT; i++) {
        uint8_t id = _drv_servo_ids[i];
        servo_set_mode(id, BS_MOTOR_MODE, 0);  // Mode to 'Position' with a speed of 0
    }
    // Move them to neutral positions slowly, so as to not cause undue force or movement.
    servos_zero_position(2000);
    // Power on all of the servos
    servo_load(SERVO_ALL_ID);
}

void servos_stop(void) {
    rover_aux_pwr_on(false);
}


void servos_module_init(void) {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("servos_module_init already called");
    }
    _initialized = true;

    _velo_last = 0;
    _rip_lfrr_pos = DIRECTIONAL_SERVO_POS_CENTER + roundf(rover_rip_angl() / servo_rads_per_unit);
    _rip_rflr_pos = DIRECTIONAL_SERVO_POS_CENTER - roundf(rover_rip_angl() / servo_rads_per_unit);
    // Calculate the center/middle wheel speed factor for rotate-in-place
    _rip_spdfctr_c = rover_cp_t() / rover_cp_cnr_d(); // half the track divided by center to corner.

    // Set the servos to Zero positions, but don't actually move the servos.
    servos_zero_position(-1);

    servo_module_init();
}
