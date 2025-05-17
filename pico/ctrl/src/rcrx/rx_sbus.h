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

#include "piosm.h"

#include "stdint.h"
#include "hardware/pio.h"

/** @brief SBUS Message Length */
#define SBUS_MSG_LEN 25

extern void pio_rx_sbus_deinit(PIO pio, uint sm, int offset);

extern pio_sm_cfg pio_rx_sbus_init(PIO pio, uint sm, uint pin, uint baud);

/**
 * @brief Read an entire SBUS message from a PIO-SM (for DEBUG/TEST)
 *
 * This reads an entire message, blocking until it is completely received.
 * This should only be used for debugging or test, as it blocks processor
 * execution until the entire message is received (3ms minimum).
 *
 * @param pio PIO block
 * @param sm State Machine within the block
 * @param buf Character buffer large enough to hold the entire message
 */
extern void pio_rx_sbus_msgget(PIO pio, uint sm, volatile unsigned char* buf);

#ifdef __cplusplus
}
#endif
#endif // RX_SBUS_H_
