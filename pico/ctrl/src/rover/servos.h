/**
 * @brief Serial Bus Servo group control.
 * @ingroup servo
 *
 * Controls a group of HiWonder Serial Bus servos for the rover.
 * They are configured as follows:
 *
 * LF_DIR                              RF_DIR
 * LF_DRIVE                            RF_DRIVE
 *
 *
 * LM_DRIVE                            RM_DRIVE
 *
 *                BOGIE_PIVOT
 * LR_DRIVE                            RR_DRIVE
 * LR_DIR                              RR_DIR
 *
 * LF_DIR   : Turns the Left-Front drive wheel from -90°~0~90° (125~0~875)
 * LF_DRIVE : Drives the Left-Front drive wheel with a speed from -1000 to 0 to +1000
 * LM_DRIVE : Drives the Left-Middle drive wheel
 * LR_DRIVE : Drives the Left-Rear drive wheel
 * LR_DIR   : Turns the Left-Rear drive wheel from -90°~0~90° (125 to 0 to 875)
 * RF_DIR   : Turns the Right-Front drive wheel from -90°~0~90° (125 to 0 to 875)
 * RF_DRIVE : Drives the Right-Front drive wheel with a speed from -1000 to 0 to +1000
 * RM_DRIVE : Drives the Right-Middle drive wheel
 * RR_DRIVE : Drives the Right-Rear drive wheel
 * RR_DIR   : Turns the Right-Rear drive wheel from -90°~0~90° (125 to 0 to 875)
 *
 * BOGIE_PIVOT : Reads, and can drive, the bogie pivot arm angle
 *
 * The directional servos can actually go from -120° to 0 to +120° (0~500~1000), but
 * that doesn't make sense in the operation of the rover, so the limits are set to
 * -90° to 90°.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _SERVOS_H_
#define _SERVOS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct pair_uint16_ {
    uint16_t a;
    uint16_t b;
} pair_uint16_t;

/**
 * @brief Indicates if the rover is in Rotate-In-Place mode.
 *
 * @return true Rover is positioned for RIP
 * @return false Rover is positioned for 'normal' travel
 */
extern bool servos_rip();

/**
 * @brief Position the directional servos for a Rotate-In-Place manuever.
 * @ingroup servos
 *
 * For Rotate-In-Place (spin) the front steering needs to be toe-in and the
 * rear steering needs to be toe-out. This is done to an angle, such that,
 * the axles are pointing to the drive center point (Logical-Center ~ LC).
 */
extern void servos_rip_position();

/**
 * @brief Drive the wheels for rotate-in-place. Negative value rotates
 *      counter-clockwise, positive value rotates clockwise.
 *
 * Note: The speed factors are 1 for the corners and a fraction for the center/middle
 *      wheels which is calculated in module init.
 *
 * @param rsp Rotation speed from the RC (-10000 to 0 to 10000)
 */
extern void servos_rip_speed(int16_t rsp);

/**
 * @brief Set the vehicle velocity. This call is bypassed if the rover is
 *      set to RIP.
 *
 * This uses the current wheel arc radii to calculate the speed of each
 * drive wheel. This method should be used when the velocity is being adjusted
 * for the current yaw setting, as setting the vehicle yaw changes the drive
 * wheel arc radius. When both the velocity and yaw are changing, `servo_yaw_set`
 * should be used.
 *
 * @see `servo_yaw_set(uint16_t yaw, int16_t velo)`
 *
 * @param velo The requested velocity, in servo drive units, within the speed limits.
 */
extern void servos_velocity_set(int16_t velo);

/**
 * @brief Get the vehicle yaw value.
 *
 * @return uint16_t Value in servo position units.
 */
extern uint16_t servos_yaw();

/**
 * @brief The limits, in servo position units, for the minimum and maximum
 * vehicle yaw values allowed. The vehicle yaw is limited by the physical
 * servo not being allowed to move past 90 degrees.
 *
 * This is for the 'logical bicycle model' and it generates the maximum
 * yaw (steering) angle for the left and right side.
 *
 * @return pair_uint16_t `a` is the minimum allowed and `b` is the maximum allowed
 */
extern pair_uint16_t servos_yaw_limits();

/**
 * @brief Set the vehicle yaw value and velocity.
 *
 * The 'vehicle yaw' is the yaw value for the logical bicycle model used to
 * control the rover. The 'vehicle velocity' is the velocity of logical bicycle
 * model used to control the rover. Setting these values calculates the actual
 * yaw values for the corner directional servos and commands them to move to those
 * locations. Since changing the yaw also changes the travel arc radius, the
 * drive servo speeds must be changed at the same time. Therefore this command
 * also calculates the drive servo adjusted speeds from the requested vehicle
 * velocity.
 *
 * It is possible to just set the velocity.
 * @see `servos_velocity_set(int16_t velo)`
 *
 * @param value The vehicle yaw value to set. The value is limited to the lyaw limits.
 */
extern void servos_yaw_set(uint16_t value, int16_t velo);

/**
 * @brief Position the directional servos to the zero (straight-forward/backward) position
 * and set the drive speed based on the last velocity that was set.
 *
 * @param time The millisecond time to take to move the servos into position or less than
 *      zero to just calculate and set the values without moving servos.
 */
extern void servos_zero_position(int16_t time);


/**
 * @brief Housekeeping for the Servos module.
 * @ingroup servo
 *
 * This performs regular housekeeping for the Servos Module.
 * It is expected to be called every ~16ms by the Directional Control System.
 */
extern void servos_housekeeping(void);

/**
 * @brief Starts the various servos on the rover.
 * @ingroup servo
 *
 * This should be called after the messaging system is up and running.
 * This reads the position, sets the position, and powers up all of the
 * servos on the rover.
 */
extern void servos_start(void);

/**
 * @brief Initialize the Serial Bus Servos (group) control module.
 * @ingroup servo
 */
extern void servos_module_init(void);


#ifdef __cplusplus
    }
#endif
#endif // _SERVOS_H_
