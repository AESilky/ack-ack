/*
    Remote Control Receive.

    This module reads manual remote control signals from a Spektrum or FrSKY
    radio control receiver. The Spektrum receiver is connected using SRXL2.
    The FrSKY receiver is connected using SBUS (non-inverted).

    The module attempts to determine the type SRXL2 or SBUS and the BAUD rate,
    400,000 or 115,200 (SRXL2), or 100,000 (SBUS).

    Once the type and BAUD are determined, the module starts receiving channel
    data and making it available to the system.


    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

    Portions Copyright(c) 2020 Raspberry Pi(Trading) Ltd.
    SPDX - License - Identifier: BSD - 3 - Clause

*/

#ifndef RCRX_H_
#define RCRX_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "rcrx_t.h"

#include "hardware/pio.h"

#include <stdbool.h>
#include <stdint.h>

extern void rcrx_clear_ch_state();

extern const rcrx_state_t* rcrx_get_ch_state();

extern rxprotocol_t rcrx_get_protocol();

extern const char* rcrx_get_type_name(rxprotocol_t type);

/**
 * @brief Get the received message/packet count.
 *
 * This is the number of messages/packets received since the protocol & BAUD
 * were detected and reception was enabled.
 *
 * This value can be useful for a heartbeat routine to verify that messages
 * are being received.
 *
 * @return uint64_t The number of messages/packets received.
 */
extern uint64_t rcrx_get_rx_cnt();

/**
 * @brief Print the RC Channels state.
 *
 * @param hl_chg Highlight changed channels if true.
 */
extern void rcrx_print_ch_state(bool hl_chg);

extern void rcrx_module_init();

extern void rcrx_start();

#ifdef __cplusplus
    }  // extern "C"
#endif
#endif // RCRX_H_
