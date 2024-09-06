/**
 * Servo control functions.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef _SERVO_CTRL_H_
#define _SERVO_CTRL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// Default count values
#define DECIDEGREE_COUNT_DEF 9 // 9µs per deg
#define NEUTRAL_COUNT_DEF 15000
#define PERIOD_COUNT_DEF 200000
#define SERVO_MAX_COUNT_DEF 23000
#define SERVO_MIN_COUNT_DEF 4000


typedef struct _servoctl_ {
    uint32_t zero_count;        // The count for the neutral position
    uint32_t decidegree_count;  // The count value for a 1/10º movement
    uint32_t min_count;         // The minimum count allowed for the servo
    uint32_t max_count;         // The maximum count allowed for the servo
    uint32_t pos;               // The current position
    bool enabled;               // Enabled state
} servoctl_t;

/**
 * @brief Disable a servo (turn off the pulse generator).
 *
 * @param servo_num The servo number (0 based)
 */
extern void servo_disable(int servo_num);

/**
 * @brief Enable a servo (turn on the pulse generator).
 *
 * @param servo_num The servo number (0 based)
 */
extern void servo_enable(int servo_num);

/**
 * @brief Get the servo control information.
 *
 * @param servo_num The servo number (0 based)
 * @param servo servoctl_t pointer to fill in.
 */
extern void servo_get(int servo_num, servoctl_t *servo);

/**
 * @brief Set the servo control information.
 *
 * @param servo_num The servo number (0 based)
 * @param servo servoctl_t pointer to copy from.
 */
extern void servo_set(int servo_num, servoctl_t *servo);

/**
 * @brief Enable or disable a servo (turn on/off the pulse generator).
 *
 * @param servo_num The servo number (0 based)
 * @param enabled Bool state
 */
extern void servo_set_enabled(int servo_num, bool enabled);

/**
 * @brief Set the angle of the servo (in tenths of a degree +- from 0).
 *
 * Most servos can move +- 90º from 0.
 *
 * @param servo_num The servo number (0 based)
 * @param decidegree The angle in 1/10 degrees +- from 0
 */
extern void servo_set_angle(int servo_num, int32_t decidegree);

/**
 * @brief Initialize the Servo Control module.
 */
extern void servo_module_init();

#ifdef __cplusplus
    }
#endif
#endif // _SERVO_CTRL_H_
