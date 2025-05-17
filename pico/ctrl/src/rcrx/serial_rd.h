/*
    Serial Read PIO initialization.

    Copyright 2025 AESilky (SilkyDESIGN)
    SPDX-License-Identifier: MIT

*/
#ifndef SERIAL_RD_H_
#define SERIAL_RD_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "piosm.h"

#include "stdint.h"
#include "hardware/pio.h"

/**
 * @brief De-initialize the PIO State Machine used to read serial data.
 *
 * @param pio The PIO block
 * @param sm  The State Machine
 * @param offset The offset the program was loaded at
 * @param inverse True if the serial is inverted
 */
extern void pio_serial_rd_deinit(PIO pio, uint sm, int offset, bool inverse);

/**
 * @brief Initialize the PIO State Machine used to read serial data.
 *
 * @param pio The PIO block
 * @param sm The State Machine
 * @param pin The GPIO Pin
 * @param baud The BAUD rate for the serial data
 * @param inverse True if the serial is inverted
 * @return int Offset the PIO program is loaded at
 */
extern pio_sm_cfg pio_serial_rd_init(PIO pio, uint sm, uint pin, uint baud, bool inverse);

#ifdef __cplusplus
}
#endif
#endif // SERIAL_RD_H_
