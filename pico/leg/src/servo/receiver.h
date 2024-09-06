/**
 * Servo control functions.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef _RECEIVER_CTRL_H_
#define _RECEIVER_CTRL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct _rc_channel_ {
    uint32_t zero_count;        // The count for the neutral position
    uint32_t decidegree_count;  // The count value for a 1/10º movement
    uint32_t ns;                // The current µs value
    uint32_t pos;               // The last position
    bool valid_pos;             // Flag to indicate that `pos` is valid (has been calculated)
    bool enabled;               // Flag to indicate that the channel is enabled
} rc_channel_t;

/**
 * @brief Disable a receiver channel.
 *
 * @param channel_num The channel number (0 based)
 */
extern void channel_disable(int channel_num);

/**
 * @brief Enable a receiver channel.
 *
 * @param channel_num The channel number (0 based)
 */
extern void channel_enable(int channel_num);

/**
 * @brief Get the channel value information.
 *
 * @param channel_num The channel number (0 based)
 * @param channel rc_channel_t pointer to fill in.
 */
extern void channel_get(int channel_num, rc_channel_t *channel);

/**
 * @brief Get the angle of the channel (in tenths of a degree +- from 0).
 *
 * @param channel_num The channel number (0 based)
 * @return decidegree The angle in 1/10 degrees +- from 0
 */
extern int32_t channel_get_angle(int channel_num);

/**
 * @brief Get the nanosecond value of the channel.
 *
 * @param channel_num The channel number (0 based)
 * @param return ns absolute
 */
extern uint32_t channel_get_ns(int channel_num);

/**
 * @brief Set the channel value information.
 *
 * This is primarily intended to set the conversion values for a channel
 * rather than using the defaults.
 *
 * @param channel_num The channel number (0 based)
 * @param channel rc_channel_t pointer to copy from.
 */
extern void channel_set(int channel_num, rc_channel_t *channel);

/**
 * @brief Set the deci-degree conversion value for the channel.
 *
 * The deci-degree conversion value is the number of µs in 1/10 degree.
 *
 * @param channel_num The channel number (0 based)
 * @param decideg_value The value to use to convert µs to 0.1º value
 */
extern void channel_set_cnv_decideg(int channel_num, int32_t decideg_value);

/**
 * @brief Set the zero degree µs value for the channel.
 *
 * The zero degree value is used, along with the decideg value, to calculate the
 * position from the µs value.
 *
 * @param channel_num The channel number (0 based)
 * @param zero_value The µs value for 0º
 */
extern void channel_set_cnv_zero(int channel_num, int32_t zero_value);

/**
 * @brief Set the enabled state (enabled/disabled) for a receiver channel.
 *
 * @param channel_num The channel number (0 based)
 * @param enabled Enabled (true) or disabled (false)
 */
extern void channel_set_enabled(int channel_num, bool enabled);

/**
 * @brief Initialize the Radio Control Receiver module.
 */
extern void receiver_module_init();

#ifdef __cplusplus
    }
#endif
#endif // _RECEIVER_CTRL_H_
