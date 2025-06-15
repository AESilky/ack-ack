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

#define DIRECT_CTRL_SEL_CH (14-1) // Channel data is 0-based

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
 * @return uint8_t Channel number
 */
extern uint8_t dcs_rc_dcch();

/**
 * @brief Set the 'Direct Control' selection channel.
 *
 * @param channel
 */
extern void dcs_rc_dcch_set(uint8_t channel);

/**
 * @brief Start the RC processing for the DCS
 */
extern void dcs_rc_start();

/**
 * @brief Initialize the module
 */
extern void dcs_rc_module_init();

