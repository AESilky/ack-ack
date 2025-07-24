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
 * @brief Get the latest and previous bit values read from the sensor bank.
 * @ingroup sensbank
 *
 * @return sensbank_cah_t Bit values of the 8 sensor inputs (.bits) and previous (.prev_bits).
 */
extern sensbank_cah_t sensbank_get(void);

/**
 * @brief Perform sensor housekeeping update.
 *
 * This should be called by the main core processing. This module does
 * not register a Periodic/Housekeeping message handler.
 */
extern void sensbank_update();

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
