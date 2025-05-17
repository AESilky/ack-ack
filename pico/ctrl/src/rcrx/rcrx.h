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

extern const char* get_rxtype_name(rxprotocol_t type);

extern void rcrx_module_init();

extern void rcrx_start();

#ifdef __cplusplus
    }  // extern "C"
#endif
#endif // RCRX_H_
