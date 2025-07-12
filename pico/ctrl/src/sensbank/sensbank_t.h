/**
 * @brief Multiplexed Sensor - SENSBANK - Functionality Types.
 * @ingroup sensbank
 *
 * Data types and structures for the sensbank.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SENSBANK_T_H_
#define SENSBANK_T_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Pair of bytes that have the current and previous sensor bytes.
 *
 * Used to read sensor data into and also to publish a sensor change message.
 */
typedef struct SENSBANK_CAH_ {
    /** The current sensor bits */
    uint8_t bits;
    /** The previous sensor bits */
    uint8_t prev_bits;
} sensbank_cah_t;

#ifdef __cplusplus
    }
#endif
#endif // SENSBANK_T_H_

