/**
 * @brief Multiplexed Sensor - SENSBANK - Functionality.
 * @ingroup sensbank
 *
 * Monitors the sensbank and notifies the HWOS of changes.
 *
 * Copyright 2023-24 AESilky
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
 * @brief Get the latest bit values read from the sensor bank.
 * @ingroup sensbank
 *
 * @return uint8_t Bit values of the 8 sensor inputs.
 */
extern uint8_t sensbank_get(void);

/**
 * @brief Get the (possible) changes in the sensor bank.
 * @ingroup sensbank
 *
 * @return sensbank_chg_t Structure containing the latest and previous bits.
 */
extern sensbank_chg_t sensbank_get_chg(void);

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
