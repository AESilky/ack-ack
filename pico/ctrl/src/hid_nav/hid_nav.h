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
extern void hid_update_sensbank(sensbank_cah_t sb);

/**
 * @brief Starts the HID and NAV.
 * @ingroup hid_nav
 *
 * This should be called after the messaging system is up and running.
 */
extern void start_hid_nav(void);

#ifdef __cplusplus
    }
#endif
#endif // HID_H_
