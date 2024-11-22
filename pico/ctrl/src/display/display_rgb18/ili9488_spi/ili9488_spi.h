/**
 * ILI9488 320x480 Color LCD functionality interface through SPI
 * (used on the 3.5" display)
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */
#ifndef _ILI9488_SPI_H_
#define _ILI9488_SPI_H_
#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

#define ILI9488_WIDTH 320    // -  ILI9488 display width
#define ILI9488_HEIGHT 480   // -  ILI9488 display height

#define ILI9488_ID_MODEL1 94
#define ILI9488_ID_MODEL2 88

extern const uint8_t ili9488_init_cmd_data[];

#ifdef __cplusplus
 }
#endif
#endif  // _ILI9488_SPI_H_
