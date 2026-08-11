/**
 * @brief Multiplexed Sensor - SENSBANK - Functionality.
 * @ingroup sensbank
 *
 * Monitors the sensbank and notifies the HWRT of changes.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SENSBANK_H_
#define SENSBANK_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "sensbank_t.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Enable/disable scanning of the sensors. When disabled, S0 is
 * selected (the sensor board doesn't use S0).
 *
 * @param enable True to enable, false to disable
 */
extern void sensbank_enable(bool enable);

/**
 * @brief The acceptable delta between distance reads to be considered valid.
 * @ingroup sensbank
 *
 * When reading the Sonar and LiDAR it is possible to get false reads. This
 * delta is considered acceptable between two consecutive reads to be a
 * valid distance read.
 *
 * @see sensbank_dist_delta_accept_set(delta) to set the value.
 *
 * @return uint16_t The current acceptable delta in centimeters
 */
extern uint16_t sensbank_dist_delta_accept();

/**
 * @brief Set the acceptable delta between distance reads.
 *
 * @param delta The acceptable delta in centimeters
 */
extern void sensbank_dist_delta_accept_set(uint16_t delta);

/**
 * @brief Get the latest distance to obstacle values.
 *
 * @return sensbank_dist_t
 */
extern sensbank_dist_t sensbank_dist_get(void);

/**
 * @brief Get the latest and previous bit values read from the sensor bank.
 * @ingroup sensbank
 *
 * @return sensbank_cah_t Bit values of the 8 sensor inputs (.bits) and previous (.prev_bits).
 */
extern sensbank_cah_t sensbank_get(void);

/**
 * @brief Tests for the sensor to be on.
 * @ingroup sensbank
 *
 * This method is helpful compared to checking the bits manually, as it deals
 * with the fact that the sensor bits are 0 when the sensor is on.
 *
 * @param sensor_bit The sensor to check
 * @return true The sensor is 'ON' (could be 0 or 1 depending on the sensor)
 * @return false The sensor is 'OFF'
 */
extern bool sensbank_sensor_on(uint8_t sensor_bit);

/**
 * @brief Perform sensor housekeeping update.
 *
 * This should be called by the main core processing. This module does
 * not register a Periodic/Housekeeping message handler.
 */
extern void sensbank_update(void);

/**
 * @brief Starts reading the Sensor Bank.
 * @ingroup sensbank
 *
 * This should be called after the messaging system is up and running.
 */
extern void sensbank_start(void);

/**
 * @brief Initialize the Sensor Bank.
 * @ingroup sensbank
 */
extern void sensbank_module_init(void);


#ifdef __cplusplus
    }
#endif
#endif // SENSBANK_H_
