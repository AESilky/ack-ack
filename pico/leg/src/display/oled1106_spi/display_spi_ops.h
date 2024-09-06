/**
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _DISPLAY_SPI_OPS_H_
#define _DISPLAY_SPI_OPS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef enum _op_cmd_data {
    DISP_OP_CMD,
    DISP_OP_DATA,
} op_cmd_data_t;


extern void disp_op_begin(op_cmd_data_t cd);

extern void disp_op_end();

extern int disp_write(uint8_t data);

extern int disp_write_buf(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif
#endif // _DISPLAY_SPI_OPS_H_

