/*!
 * \brief Drive Control System use of the Radio Control.
 * \ingroup dcs
 *
 * The DCS uses the Radio Control (RC) in two ways:
 * 1. When 'Direct Control' is enabled, the DCS uses channel values to drive the rover
 * 2. When not enabled, the DCS sends RC channel values to the host
 *
 * The 'Direct Control' mode is controlled by a switch-channel from the RC. This allows
 * a human controller to select 'Direct Control' mode using a switch on the transmitter.
 * The switch defaults to CH-14, which is a two-position switch on the TARANIS TX (using
 * default mixing in OpenTX). The channel to use can be set using a method call.
 *
 * This module tracks the state of the 'Direct Control' from the Radio Control. However,
 * it is up to the DCS to choose to enable direct control or not.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>

// RC Channels defaults (Channel data is 0-based)
#define CH_DIRECT_CTRL_SEL (13)     // CH-14 (Left 2-Pos SW)
#define CH_FWD_ROT_REV (9)          // CH-10 (Left long 3-Pos SW)
#define CH_YAW (0)                  // CH-1 (Left Stick)
#define CH_THROTTLE (2)             // CH-3 (Left Stick)

/** @brief Forward-Rotate-Reverse */
typedef enum DCS_FRR_ {
    DCS_FRR_FORWARD,
    DCS_FRR_ROTATE,
    DCS_FRR_REVERSE,
} dcs_frr_t;

/** @brief Combined Yaw (Steering) and Throttle (Velocity/Speed) values */
typedef struct DCS_YT_ {
    uint16_t yaw;
    uint16_t throttle;
} dcs_yt_t;

/**
 * @brief Get the state of the 'Direct Control' flag from the RC system.
 *
 * @return true Direct Control is set
 * @return false Direct Control is clear
 */
extern bool dcs_rc_direct_ctrl();

/**
 * @brief Get the 'Direct Control' channel number.
 *
 * @return uint8_t Channel number (0-based)
 */
extern uint8_t dcs_rc_dcch();

/**
 * @brief Set the 'Direct Control' selection channel.
 *
 * @param channel 0-based channel number
 */
extern void dcs_rc_dcch_set(uint8_t channel);

/**
 * @brief Get the 'Forward-Rotate-Reverse' control channel.
 *
 * @return uint8_t Channel number (0-based)
 */
extern uint8_t dcs_rc_frrch();

/**
 * @brief Set the 'Forward-Rotate-Reverse' control channel.
 *
 * @param channel 0-based channel number
 */
extern void dcs_rc_frrch_set(uint8_t channel);

/**
 * @brief Get the state of the 'Forward-Rotate-Reverse' control from the RC system.
 *
 * @return dcs_frr_t One of: DCS_FORWARD, DCS_ROTATE, DCS_REVERSE
 */
extern dcs_frr_t dcs_rc_fwd_rot_rev();

/**
 * @brief Steering and Throttle values.
 *
 * @return dcs_yt_t int16_t Steering and uint16_t Throttle
 */
extern dcs_yt_t dcs_rc_yt();

/**
 * @brief Get the Throttle channel
 *
 * @return uint8_t 0-based channel number
 */
extern uint8_t dcs_rc_thrtch();

/**
 * @brief Set the Throttle channel
 *
 * @return uint8_t 0-based channel number
 */
extern void dcs_rc_thrtch_set(uint8_t channel);

/**
 * @brief Throttle value (rolling average), adjusted to 0 to throttle_limit.
 *
 * @return uint16_t 0 to throttle_limit adjusted servo speed value
 */
extern uint16_t dcs_rc_throttle();

/**
 * @brief Get the Yaw (Steering) channel
 *
 * @return uint8_t 0-based channel number
 */
extern uint8_t dcs_rc_yawch();

/**
 * @brief Set the Yaw (Steering) channel.
 *
 * @param channel 0-based channel number
 */
extern void dcs_rc_yawch_set(uint8_t channel);

/**
 * @brief Yaw value (rolling average), adjusted to yaw_min ~ 0 ~ yaw_max.
 *
 * @return uint16_t servo value (500 is center)
 */
extern uint16_t dcs_rc_yaw();

/**
 * @brief Get the maximum allowed yaw value (servo units).
 *
 * @return uint16_t maximum yaw value
 */
extern uint16_t dcs_rc_yawmax();

/**
 * @brief Set the maximum allowed yaw value (servo units)
 *
 * Used to calculate the yaw value from the Yaw (Steering) channel.
 *
 * @param value The maximum value to use (in servo control units)
 */
extern void dcs_rc_yawmax_set(uint16_t value);

/**
 * @brief Get the minimum allowed yaw value (servo units).
 *
 * @return uint16_t minimum yaw value
 */
extern uint16_t dcs_rc_yawmin();

/**
 * @brief Set the minimum allowed yaw value (servo units)
 *
 * Used to calculate the yaw value from the Yaw (Steering) channel.
 *
 * @param value The minimum value to use (in servo control units)
 */
extern void dcs_rc_yawmin_set(uint16_t value);

/**
 * @brief Get the raw yaw value from the RC.
 *
 * @return int16_t Raw yaw value (-10000~0~10000)
 */
extern int16_t dcs_rc_yaw_raw();

/**
 * @brief Start the RC processing for the DCS
 */
extern void dcs_rc_start();

/**
 * @brief Initialize the module
 */
extern void dcs_rc_module_init();

