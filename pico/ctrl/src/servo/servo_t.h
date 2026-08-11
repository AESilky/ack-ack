/**
 * @brief Serial Bus Servo control.
 * @ingroup servo
 *
 * Controls a collection of HiWonder Serial Bus servos.
 * This file contains the datatypes (only)
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SERVO_T_H_
#define SERVO_T_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

/** @brief Servo Position Unit is 0.24° */
#define SERVO_DEG_PER_UNIT 0.24

typedef enum BUS_SERVO_MODE_ {
    BS_POSITION_MODE = 0,
    BS_MOTOR_MODE = 1
} servo_mode_t;

/** @brief ID to broadcast a command to all servos */
#define BS_BROADCAST_ID 254
/** @brief ID to send a command to all servos (alias of `BS_BROADCAST_ID`)*/
#define SERVO_ALL_ID BS_BROADCAST_ID


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
    uint8_t id;                 // Servo ID
    bs_rx_status_t _rxstatus;   // Status received from the servo
} servo_t;
#define SERVO_NONE ((servo_t*)0)

typedef struct SERVO_PARAMS_ {
    uint8_t servo_id;
    uint16_t pos;
    uint16_t time;
} servo_params_t;


#ifdef __cplusplus
    }
#endif
#endif // SERVO_T_H_
