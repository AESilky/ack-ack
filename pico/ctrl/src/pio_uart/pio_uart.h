/*
    UART Receive implemented with the PIO. This allows more serial devices
    to be read from than the two (hard) UARTs that the Pico contains.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

    Portions Copyright(c) 2020 Raspberry Pi(Trading) Ltd.
    SPDX - License - Identifier: BSD - 3 - Clause

*/

#ifndef PIO_UART_H_
#define PIO_UART_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "hardware/pio.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize a PIO StateMachine to be used as a Serial UART Receiver.
 *
 * This configures a PIO StateMachine to receive serial data on a GPIO pin.
 *
 * @param pio The PIO to use
 * @param sm The StateMachine of the PIO
 * @param offset The program offset to use
 * @param pin The GPIO pin to receive data on
 * @param baud The BAUD rate to receive data at
 */
extern void pio_uart_rx_init(PIO pio, uint sm, uint offset, uint pin, uint baud);

#ifdef __cplusplus
    }  // extern "C"
#endif
#endif // PIO_UART_H_
