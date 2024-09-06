/**
 * Copyright 2023-24 AESilky
 * Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "display_ops.h"
#include "system_defs.h"

#include "hardware/spi.h"

#define DISPLAY_DC_CMD 0
#define DISPLAY_DC_DATA 1

/**
 * Set the chip select for the display.
 *
 */
static void _cs(bool sel) {
    if (sel) {
        gpio_put(SPI_DISP_CS, SPI_CS_ENABLE);
    }
    else {
        gpio_put(SPI_DISP_CS, SPI_CS_DISABLE);
    }
}

/**
 * Set the data/command select for the display.
 *
 */
static void _command_mode(bool cmd) {
    if (cmd) {
        gpio_put(SPI_DISP_CD, DISPLAY_DC_CMD);
    }
    else {
        gpio_put(SPI_DISP_CD, DISPLAY_DC_DATA);
    }
}

void disp_op_begin(op_cmd_data_t cd) {
    if (cd == DISP_OP_CMD) {
        _command_mode(true);
    }
    else {
        _command_mode(false);
    }
    _cs(true);
}

void disp_op_end() {
    _cs(false);
}

int disp_write(uint8_t data) {
    return (spi_write_blocking(SPI_DISP_SDC_DEVICE, &data, 1));
}

int disp_write_buf(const uint8_t* data, size_t len) {
    return (spi_write_blocking(SPI_DISP_SDC_DEVICE, data, len));
}
