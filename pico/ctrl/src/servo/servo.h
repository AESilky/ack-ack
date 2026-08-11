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

#include "cmt/cmt_t.h"

/**
 * @brief The number of degrees per position unit.
 */
extern const double servo_degs_per_unit;

/**
 * @brief Factor used to convert radians to a position value.
 */
extern const double servo_rads_to_posv_fctr;

/**
 * @brief The number of radians per position unit.
 */
extern const double servo_rads_per_unit;

/**
 * @brief Enable the motor of the servo (it will drive the control arm)
 *
 * @param id Servo ID (can be `servo_all`)
 */
extern bool servo_load(uint8_t id);

/**
 * @brief Move a positional servo to a given 'position' and take 'time'
 * to get there.
 *
 * @param id ID of servo to control
 * @param position Position (0-1000) or (0-1500) depending on servo
 * @param time ms 0~30000ms
 */
extern bool servo_move(uint8_t id, int16_t position, uint16_t time);

/**
 * @brief Move a group of positional servos to specified positions and take
 * 'time' to get there.
 *
 * This sends the location to each of the servos without moving them. Once
 * the position is sent to all of the servos, they are instructed to move
 * to the specified location.
 *
 * @see `servo_notify_on_ready(msg_handler_fn fn, uint8_t core)` for registering
 * a function for notification in the case that `false` is returned.
 *
 * @param id Array of servo IDs to control
 * @param position Array of positions, one for each servo
 * @param time The time for the servo to take to get to the position
 * @param count The number of servos (size of the arrays)
 * @return true The servos were controlled
 * @return false The control bus is busy, the call should be re-tried.
 */
extern bool servo_move_group(uint8_t id[], uint16_t position[], uint16_t time, int count);

/**
 * @brief Register a function to be notified when the servo waiting status
 * has been cleared and a command can be sent to the servo bus.
 *
 * Once the function is notified the function is cleared. To be notified
 * again, a function must be registered again.
 *
 * Note: The servo bus does not need to be busy to register a function.
 *
 * @param fn A (CMT) Message Handler Function that will be posted to
 * @param core The core to post to
 * @return true The function was registered
 * @return false The function could not be registered (one is already registered)
 */
extern bool servo_notify_on_ready(msg_handler_fn fn, uint8_t core);

/**
 * @brief Clear the function waiting to be notified.
 *
 * @param fn The function that was registered
 */
extern void servo_notify_on_ready_clear(msg_handler_fn fn);

/**
 * @brief Convert a servo position value to a radian value.
 *
 * @param posv Servo position value
 * @param center The servo position value representing 0 (typ 500 or 750)
 * @return float Radian value (0 is center)
 */
static inline float servo_posv_to_rads(uint16_t posv, uint16_t center) {
    // Adjust `posv` such that 0 is center and multiply by radian per unit factor
    return (float)((double)(posv - center) * servo_rads_per_unit);
}

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
 * @brief Convert a radian value to a Servo Position Delta value.
 *
 * Note: This is a position movement (delta) value. Not an absolute
 *      position.
 *
 * @param rads Radian value to convert
 * @return uint16_t Servo Position Delta value
 */
extern uint16_t servo_rads_to_posd(float rads);

/**
 * @brief Shortcut for setting the servo mode to 'motor' and setting the speed.
 * @ingroup servo
 *
 * @param id ID of servo to control
 * @param speed Speed from -1000 to 0 to +1000 (<0 is reverse, >0 is forward)
 */
extern bool servo_run(uint8_t id, int16_t speed);

/**
 * @brief Run a group of 'motor' servos to specified speeds.
 *
 * This sets the servos to motor mode and sets the speed of each of the servos.
 * Unlike position servos, motor servos don't support waiting for a 'run' command
 * to change the speed, but this method sends the command and speed to the servos
 * with minimal overhead between the transmissions.
 *
 * Using this method to control multiple drive servos should be preferred over
 * calling `servo_run` or `servo_set_mode` repeatedly.
 *
 * @see servo_run(servo_t* servo, int16_t speed)
 * @see servo_set_mode(servo_t* servo, servo_mode_t mode, int16_t speed)
 * @see `servo_notify_on_ready(msg_handler_fn fn, uint8_t core)` for registering
 * a function for notification in the case that `false` is returned.
 *
 * @param id Array of servo IDs to control
 * @param speed Array of speeds, one for each servo
 * @param count The number of servos (size of the arrays)
 * @return true The servos were controlled
 * @return false The control bus is busy, the call should be re-tried.
 */
extern bool servo_run_group(uint8_t id[], int16_t speed[], int count);

/**
 * @brief Function that sets (changes) the ID of a servo.
 *
 * Typically, this operation is done on the bench using the HiWonder
 * utility and servo test tool.
 *
 * @param oldID
 * @param newID
 * @return true
 * @return false
 */
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
 * @brief Set the limits of a position mode servo.
 *
 * Though the full limit of a position mode servo is 0 to 500 to 1000,
 * the servo can be configured to a smaller angle. A value of 1 is 0.24°.
 * The servo will not travel past these limits even if smaller or larger
 * values are sent to the servo.
 *
 * The minimum must be less than the maximum.
 *
 * @param id Servo ID
 * @param min Minimum value to allow moving to
 * @param max Maximum value to allow moving to
 * @return true If the command was able to be sent immediately
 */
extern bool servo_set_limits(uint8_t id, uint16_t min, uint16_t max);

/**
 * @brief Set the servo to position mode or motor mode.
 *
 * In position mode, the servo moves to a specified position. In motor
 * mode it rotates at a specified speed.
 *
 * @param id Servo ID
 * @param mode BS_POSITION_MODE | BS_MOTOR_MODE
 * @param speed -1000 to 1000 (used if the servo is put into motor mode)
 */
extern bool servo_set_mode(uint8_t id, servo_mode_t mode, int16_t speed);

/**
 * @brief Stop the movement of the servo.
 *
 * This can stop a single servo or with the 'ALL' Servo, all the servos
 * can be stopped at once.
 *
 * @param id ID of servo to stop (can be `servo_all`)
 * @return true
 * @return false
 */
extern bool servo_stop_move(uint8_t id);

/**
 * @brief Disable the motor of the servo (it will not drive the control arm)
 *
 * @param id ID of servo to unload (can be `servo_all`)
 */
extern bool servo_unload(uint8_t id);

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
