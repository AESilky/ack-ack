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

// Used for decoding the Sensbank Value.

/** @brief Green User Button sensor bit within Sensbank Value. */
#define BTN_GREEN_SENSOR_BIT       0x02    // Green button is on Sensor-Bit 1
/** @brief Yellow User Button sensor bit within Sensbank Value. */
#define BTN_YELLOW_SENSOR_BIT      0x04    // Yellow button is on Sensor-Bit 2

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

/**
 * @brief Distance (cm) to obstacle for Sonar0, Sonar1, and LiDAR.
 * 
 * @param sonar0 Distance in centimeters from Sonar0 (rear)
 * @param sonar0_ts Timestamp of the last valid distance read
 * @param sonar1 Distance in centimeters from Sonar1 (front)
 * @param sonar1_ts Timestamp of the last valid distance read
 * @param lidar Distance in centimeters from LiDAR (front)
 * @param lidar_ts Timestamp of the last valid distance read
 */
typedef struct SENSBANK_DIST_VALS_ {
    uint16_t sonar0;
    uint32_t sonar0_ts;
    uint16_t sonar1;
    uint32_t sonar1_ts;
    uint16_t lidar;
    uint32_t lidar_ts;
} sensbank_dist_t;

#ifdef __cplusplus
    }
#endif
#endif // SENSBANK_T_H_

