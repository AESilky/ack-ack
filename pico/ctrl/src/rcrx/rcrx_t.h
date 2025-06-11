/*
    Remote Control Receive - Typedefs (only)

    @see rcrx.h for a description of this module.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT
*/

#ifndef RCRX_T_H_
#define RCRX_T_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define RC_SYS_CHVAL_MAX 0xFFFF
#define RC_SYS_CHVAL_MID 0x8000
#define RC_SYS_CHVAL_MIN 0x0000

/**
 * @brief The RC Receiver Protocols
 */
typedef enum RXPROTOCOL_ {
    RXP_UNKNOWN = 0,
    RXP_SBUS = 1,
    RXP_SRXL2 = 2
} rxprotocol_t;

/**
 * @brief The RC Receiver Baud & Protocol
 */
typedef struct RCRX_BP_ {
    uint32_t baud;
    rxprotocol_t protocol;
} rcrx_bp_t;

/** @brief The number of channels supported */
#define RCRX_CHANNELS_SUPPORTED    16

typedef struct RCRX_CHANNEL_DATA_ {
    volatile uint16_t raw_v;     // Raw data (as received)
    volatile uint16_t v;         // Value 0x8000 = Middle (adjusted based on protocol)
} rcrx_ch_data_t;

typedef struct RCRX_STATE_ {
    volatile bool failsafe;
    volatile bool local_rx_disabled;
    volatile uint16_t local_errs_in_period;
    volatile uint32_t local_err_cnt_all;
    volatile uint32_t local_parity_err_cnt;
    volatile uint32_t frames_lost;
    volatile uint32_t msgs_processed;
    volatile int8_t rssi; // 0 means 'not connected'. Value <0 is dBm. Value >0 is %R
    rcrx_ch_data_t ch_data[RCRX_CHANNELS_SUPPORTED];
} rcrx_state_t;


#ifdef __cplusplus
    }  // extern "C"
#endif
#endif // RCRX_H_
