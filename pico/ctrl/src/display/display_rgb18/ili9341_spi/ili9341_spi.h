/**
 * ILI9341 240x320 Color LCD functionaly interface through SPI
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */
#ifndef _ILI9341_SPI_H_
#define _ILI9341_SPI_H_
#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

#define ILI9341_WIDTH 240    // -  ILI9341 display width
#define ILI9341_HEIGHT 320   // -  ILI9341 display height

#define ILI9341_ID_MODEL1 93
#define ILI9341_ID_MODEL2 41

/**
 * @brief ILI Controller initialization data.
 * @ingroup display
 */
extern const uint8_t ili9341_init_cmd_data[];

#ifdef __cplusplus
 }
#endif
#endif  // _ILI9341_SPI_H_
