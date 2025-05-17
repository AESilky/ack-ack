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
 * 
 */
typedef struct RCRX_BP_ {
    uint32_t baud;
    rxprotocol_t protocol;
} rcrx_bp_t;

#ifdef __cplusplus
    }  // extern "C"
#endif
#endif // RCRX_H_
