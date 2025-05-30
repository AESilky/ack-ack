/*
    Receive SBUS data using the PIO.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

*/
#ifndef RX_SBUS_H_
#define RX_SBUS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "rcrx_t.h"

#include "pio_sm.h"

#include "stdint.h"
#include "hardware/pio.h"

/** @brief SBUS Message Length */
#define SBUS_MSG_LEN 25


extern void rx_sbus_module_deinit();

extern void rx_sbus_start();

/**
 * @brief Initialize the module for SBUS reception.
 *
 * @param baud The baud rate to use (typically 100,000)
 * @param channel_state Pointer to a rcrx_state_t Channel State to update with received data.
 */
extern void rx_sbus_module_init(uint baud, rcrx_state_t* channel_state);


#ifdef __cplusplus
}
#endif
#endif // RX_SBUS_H_
