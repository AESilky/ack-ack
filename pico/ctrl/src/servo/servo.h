/**
 * @brief Serial Bus Servo control.
 * @ingroup servo
 *
 * Controls a HiWonder Serial Bus servo.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _SERVO_H_
#define _SERVO_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "servo_t.h"

/**
 * @brief Enable the motor of the servo (it will drive the control arm)
 *
 * @param servo Servo
 */
    extern bool servo_load(servo_t* servo);

/**
 * @brief Move a positional servo to a given 'position' and take 'time'
 * to get there.
 *
 * @param servo Servo to control
 * @param position Position (0-1000) or (0-1500) depending on servo
 * @param time ms 0~30000ms
 */
extern bool servo_move(servo_t* servo, int16_t position, uint16_t time);

/**
 * @brief Get the position (0-1000 or 0-1500) of the servo from a servo
 * status packet.
 *
 * The servo status packet would have been the data of a MSG_SERVO_STATUS
 * message that was received as the result of a `servo_position_read` command.
 *
 * @param servo The servo with a status packet to get the value from
 * @return int16_t The position (0-1000 or 0-1500) or -1 if the packet is invalid
 */
extern int16_t servo_position(servo_t* servo);

/**
 * @brief Send command to read a servo position.
 *
 * This initiates sending an appropriate servo status read command.
 * If successful, the result will be a MSG_SERVO_STATUS message with
 * the status data.
 *
 * @param servo The servo to read the status from.
 * @return true The read status command was sent
 * @return false The read status command could not be sent
 */
extern bool servo_position_read(servo_t* servo);

/**
 * @brief Shortcut for setting the servo mode to 'motor' and setting the speed.
 * @ingroup servo
 *
 * @param servo Servo to control
 * @param speed Speed from -1000 to 0 to +1000
 */
extern bool servo_run(servo_t* servo, int16_t speed);

extern bool servo_set_id(uint8_t oldID, uint8_t newID);

/**
 * @brief Indicates if a servo status (a command that reads servo data) is
 * pending the status data being received.
 *
 * When a servo command that expects data to be received from the servo is
 * executed, this status will become true until the complete data packet
 * has been received, or another command is sent.
 *
 * @return true Incoming servo data is pending
 * @return false No incoming data is pending
 */
extern bool servo_status_inbound_pending(void);

/**
 * @brief Set the servo to position mode or motor mode.
 *
 * In position mode, the servo moves to a specified position. In motor
 * mode it rotates at a specified speed.
 *
 * @param servo Servo
 * @param mode BS_POSITION_MODE | BS_MOTOR_MODE
 * @param speed -1000 to 1000
 */
extern bool servo_set_mode(servo_t* servo, servo_mode_t mode, int16_t speed);

extern bool servo_stop_move(servo_t* servo);

/**
 * @brief Disable the motor of the servo (it will not drive the control arm)
 *
 * @param servo Servo to unload
 */
extern bool servo_unload(servo_t* servo);

/**
 * @brief Get the input voltage of the servo from a servo status packet.
 *
 * The servo status packet would have been the data of a MSG_SERVO_STATUS
 * message that was received as the result of a `servo_xxx_read` command.
 *
 * @param servo The servo to get the value from
 * @return int16_t The input voltage (mV) or -1 if the packet is invalid
 */
extern int16_t servo_vin(servo_t* servo);

/**
 * @brief Send a Read Voltage In command to a servo.
 *
 * This initiates sending an appropriate servo status read command.
 * If successful, the result will be a MSG_SERVO_STATUS message with
 * the status data.
 *
 * @param servo The servo to read the status from.
 * @return true The read status command was sent
 * @return false The read status command could not be sent
 */
extern bool servo_vin_read(servo_t* servo);


/**
 * @brief Initialize the Serial Bus Servo control module.
 * @ingroup servo
 *
 * @param servo_ids Array of servo IDs that will be controlled. -1 for unused.
 */
extern void servo_module_init(void);

/**
 * @brief Start the servo operations.
 * @ingroup servo
 *
 * Should be called once messaging and other subsystems are started.
 *
 */
extern void servo_module_start(void);

#ifdef __cplusplus
    }
#endif
#endif // _SERVO_H_
