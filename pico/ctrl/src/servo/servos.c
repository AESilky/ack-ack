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
#include "servo.h"

#include "board.h"
#include "rover_info.h"
#include "cmt/cmt.h"

#include "pico/stdlib.h"



// ############################################################################
// Constants, Enumerations and Structures
// ############################################################################
//
#define DIRECTIONAL_SERVO_POS_CENTER 500
/** @brief Left-Front and Right-Rear position for Rotate-In-Place */
#define RIP_LFRR_POS ((uint16_t)(DIRECTIONAL_SERVO_POS_CENTER - 400))  // SERVO_POS_RIP
/** @brief Right-Front and Left-Rear position for Rotate-In-Place */
#define RIP_RFLR_POS ((uint16_t)(DIRECTIONAL_SERVO_POS_CENTER + 400))  // SERVO_POS_RIP

typedef enum DIRECTIONAL_SERVOS_ID_ {
    SRVDIR_LF = 0,
    SRVDIR_LR,
    SRVDIR_RF,
    SRVDIR_RR
} dir_servo_id_t;
#define DIRECTIONAL_SERVO_CNT 4

typedef enum DRIVE_SERVOS_ID_ {
    SRVDRV_LF = 0,
    SRVDRV_LM,
    SRVDRV_LR,
    SRVDRV_RF,
    SRVDRV_RM,
    SRVDRV_RR
} drv_servo_id_t;
#define DRIVE_SERVO_CNT 6

/**
 * @brief Control structure for a directional (position controlled) servo.
 */
typedef struct DIRECTIONAL_SERVO_CTRL_ {
    servo_t servo;
    dir_servo_id_t loc;
    uint16_t max_pos;
    uint16_t pos;
    uint16_t req_pos;
} dir_servo_ctrl_t;

/**
 * @brief Control structure for a drive (speed controlled) servo.
 */
typedef struct DRIVE_SERVO_CTRL_ {
    servo_t servo;
    drv_servo_id_t loc;
    int16_t speed;
} drv_servo_ctrl_t;


// ############################################################################
// Data
// ############################################################################
//
static dir_servo_ctrl_t _dir_servos[4];
static drv_servo_ctrl_t _drv_servos[6];


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
    if (servo_move(&_dir_servos[SRVDIR_LF].servo, pos, time)) {
        return true;
    }
    // Command couldn't be sent, post ourself a message to try again.
    cmt_msg_t msg;
    msg.data.servo_params.pos = pos;
    msg.data.servo_params.time = time;
    cmt_msg_init3(&msg, MSG_EXEC, MSG_PRI_NORM, _position_lf_mh);
    postHWCtrlMsg(&msg);
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
    if (servo_move(&_dir_servos[SRVDIR_LR].servo, pos, time)) {
        return true;
    }
    // Command couldn't be sent, post ourself a message to try again.
    cmt_msg_t msg;
    msg.data.servo_params.pos = pos;
    msg.data.servo_params.time = time;
    cmt_msg_init3(&msg, MSG_EXEC, MSG_PRI_NORM, _position_lr_mh);
    postHWCtrlMsg(&msg);
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
    if (servo_move(&_dir_servos[SRVDIR_LF].servo, pos, time)) {
        return true;
    }
    // Command couldn't be sent, post ourself a message to try again.
    cmt_msg_t msg;
    msg.data.servo_params.pos = pos;
    msg.data.servo_params.time = time;
    cmt_msg_init3(&msg, MSG_EXEC, MSG_PRI_NORM, _position_rf_mh);
    postHWCtrlMsg(&msg);
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
    if (servo_move(&_dir_servos[SRVDIR_LR].servo, pos, time)) {
        return true;
    }
    // Command couldn't be sent, post ourself a message to try again.
    cmt_msg_t msg;
    msg.data.servo_params.pos = pos;
    msg.data.servo_params.time = time;
    cmt_msg_init3(&msg, MSG_EXEC, MSG_PRI_NORM, _position_rr_mh);
    postHWCtrlMsg(&msg);
    return false;
}

// ############################################################################
// Public Functions
// ############################################################################
//

void servos_rip_position() {
    _position_lf(RIP_LFRR_POS, 800);
    _position_rr(RIP_LFRR_POS, 800);
    _position_lr(RIP_RFLR_POS, 800);
    _position_rf(RIP_RFLR_POS, 800);
}

void servos_zero_position() {
    _position_lf(DIRECTIONAL_SERVO_POS_CENTER, 800);
    _position_rr(DIRECTIONAL_SERVO_POS_CENTER, 800);
    _position_lr(DIRECTIONAL_SERVO_POS_CENTER, 800);
    _position_rf(DIRECTIONAL_SERVO_POS_CENTER, 800);
}


// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//

void servos_housekeeping(void) {
    // ZZZ - Temp, exercise the position servos
    static uint8_t hk_count = 0;
    static uint8_t hk_srvo = 0;
    if (++hk_count % (62 * 3) == 0) {
        switch (hk_srvo++ & 0x03) {
            case 0:
                servo_position_read(&_dir_servos[SRVDIR_LF].servo);
                break;
            case 1:
                servo_position_read(&_dir_servos[SRVDIR_LR].servo);
                break;
            case 2:
                servo_position_read(&_dir_servos[SRVDIR_RF].servo);
                break;
            case 3:
                servo_position_read(&_dir_servos[SRVDIR_RR].servo);
                break;
        }
    }
}

void servos_start(void) {
    servo_module_start();
    // Read and set all of the servos and then power them up.
    //  Initialize all of the drive servos to DRIVE mode at 0 speed.
    for (int i = 0; i < DRIVE_SERVO_CNT; i++) {
        drv_servo_ctrl_t* drvscs = &_drv_servos[i];
        int16_t speed = drvscs->speed;
        servo_run(&drvscs->servo, speed);  // Mode to 'Motor' with a speed of 0
        servo_load(&drvscs->servo);        // Power on
    }
    // For the positional servos, move them to neutral positions slowly, so as
    // to not cause undue force or movement.
    for (int i = 0; i < DIRECTIONAL_SERVO_CNT; i++) {
        dir_servo_ctrl_t* dirscs = &_dir_servos[i];
        int16_t pos = (int16_t)(dirscs->max_pos / 2);
        servo_set_mode(&dirscs->servo, BS_POSITION_MODE, 0);  // Mode to 'Position' with a speed of 0
        servo_move(&dirscs->servo, pos, 1000);  // Move to the middle and take 1 seconds to do it.
        servo_load(&dirscs->servo);  // Power on
    }
}



void servos_module_init(void) {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("servos_module_init already called");
    }
    _initialized = true;

    //  Drive
    drv_servo_ctrl_t* drvscs = &_drv_servos[SRVDRV_LF];
    drvscs->loc = SRVDRV_LF;
    drvscs->speed = 0;
    drvscs->servo.id = 10;
    drvscs = &_drv_servos[SRVDRV_LM];
    drvscs->loc = SRVDRV_LM;
    drvscs->speed = 0;
    drvscs->servo.id = 11;
    drvscs = &_drv_servos[SRVDRV_LR];
    drvscs->loc = SRVDRV_LR;
    drvscs->speed = 0;
    drvscs->servo.id = 12;
    drvscs = &_drv_servos[SRVDRV_RF];
    drvscs->loc = SRVDRV_RF;
    drvscs->speed = 0;
    drvscs->servo.id = 13;
    drvscs = &_drv_servos[SRVDRV_RM];
    drvscs->loc = SRVDRV_RM;
    drvscs->speed = 0;
    drvscs->servo.id = 14;
    drvscs = &_drv_servos[SRVDRV_RR];
    drvscs->loc = SRVDRV_RR;
    drvscs->speed = 0;
    drvscs->servo.id = 15;
    //  Directional
    dir_servo_ctrl_t* dirscs = &_dir_servos[SRVDIR_LF];
    dirscs->loc = SRVDIR_LF;
    dirscs->max_pos = 1000;
    dirscs->servo.id = 50;
    dirscs = &_dir_servos[SRVDIR_LR];
    dirscs->loc = SRVDIR_LR;
    dirscs->max_pos = 1000;
    dirscs->servo.id = 51;
    dirscs = &_dir_servos[SRVDIR_RF];
    dirscs->loc = SRVDIR_RF;
    dirscs->max_pos = 1000;
    dirscs->servo.id = 52;
    dirscs = &_dir_servos[SRVDIR_RR];
    dirscs->loc = SRVDIR_RR;
    dirscs->max_pos = 1000;
    dirscs->servo.id = 53;

    servo_module_init();
}
