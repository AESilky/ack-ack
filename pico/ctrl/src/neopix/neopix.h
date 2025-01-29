/**
 * @brief Neopixel Panel Display Driver.
 * @ingroup neopixel
 *
 * Provides graphics display on two 4x8 Neopixel panels.
 *
 * This could be made more generic, but the 4x8 panel is readily available
 * from Adafruit as a 'FeatherWing'. Restricting to the 4x8 panel and RGB
 * helps keep things much simpler.
 *
 * The idea is to provide two 'eyes' that can show expressions. Of course,
 * anything that can fit on two 4x8 panels can be displayed. This module
 * uses two consecutive 'frame buffers' and the DMA to update the two display panels.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef NEOPIX_H_
#define NEOPIX_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/** Frame Buffer size is 2 4x8 NeoPixel panels of RGB values */
#define NEOPIX_FRAME_BUF_ELEMENTS 32  // 2 * (4x8 * 3)

extern void neopix_start(void);

extern void neopix_module_init(void);

#ifdef __cplusplus
    }
#endif
#endif // NEOPIX_H_
