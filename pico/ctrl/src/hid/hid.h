/**
 * @brief Human Interface Device.
 * @ingroup hid
 *
 * Display status and provide the human interface.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef HID_H_
#define HID_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "sensbank/sensbank.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Get and display the current Sensbank status.
 * @ingroup hid
 */
extern void hid_update_sensbank(sensbank_chg_t sb);

/**
 * @brief Starts the status display.
 * @ingroup hid
 *
 * This should be called after the messaging system is up and running.
 */
extern void hid_start(void);

/**
 * @brief Initialize the Sensor Bank.
 * @ingroup hid
 */
extern void hid_module_init(void);


#ifdef __cplusplus
    }
#endif
#endif // HID_H_
