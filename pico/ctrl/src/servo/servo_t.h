/**
 * @brief Serial Bus Servo control.
 * @ingroup servo
 *
 * Controls a collection of HiWonder Serial Bus servos.
 * This file contains the datatypes (only)
 *
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SERVO_T_H_
#define SERVO_T_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "rover_info.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

/** @brief Radians to Servo Position Value (0-1000 | 0-1500). Servo Position is 0.24° */
#define SERVO_RAD_2_POS_FCTR ((180/M_PI) * 0.24)
static inline uint16_t servo_rads2pos(float rads) {
    return ((uint16_t)(rads * SERVO_RAD_2_POS_FCTR));
}
#define SERVO_POS_RIP (servo_rads2pos(ROVER_ANGL_RIP))

typedef enum BUS_SERVO_MODE_ {
    BS_POSITION_MODE = 0,
    BS_MOTOR_MODE = 1
} servo_mode_t;

#define BS_BROADCAST_ID 254


enum BS_STATUS_PACKET_OFFSETS_ {
    BSPKT_HEADER1 = 0,
    BSPKT_HEADER2,
    BSPKT_ID,
    BSPKT_LEN,
    BSPKT_CMD,
    BSPKT_DATA
};
#define BSPKT_PAYLOAD_MAX_LEN 8

typedef struct BS_RX_STATUS_ {
    uint8_t buf[BSPKT_PAYLOAD_MAX_LEN]; // Control bytes and payload of the largest response (plus checksum)
    uint8_t data_off;
    bool frame_started;
    uint8_t len;
    bool pending;
} bs_rx_status_t;

typedef struct BUS_SERVO_ {
    uint8_t id;     // Servo ID
    servo_mode_t mode;
    bs_rx_status_t _rxstatus;
} servo_t;
#define SERVO_NONE ((servo_t*)0)

typedef struct SERVO_PARAMS_ {
    uint8_t servo_id;
    uint16_t pos;     // Servo ID
    uint16_t time;
} servo_params_t;


#ifdef __cplusplus
    }
#endif
#endif // SERVO_T_H_
