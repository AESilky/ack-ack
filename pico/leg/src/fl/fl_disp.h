/**
 * leg User Interface - On the display.
 *
 * Copyright 2023-24 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef _UI_DISP_H_
#define _UI_DISP_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt/cmt.h"

#include "pico/types.h"

#include <stdint.h>

/**
 * @brief Build (or rebuild) the Functional-Level on the display.
 * @ingroup ui
 */
extern void fl_disp_build(void);

/**
 * @brief Print a string in the scrolling area of the screen.
 * @ingroup ui
 *
 * @param str The string to print.
 */
void fl_disp_puts(char* str);

#ifdef __cplusplus
}
#endif
#endif // _UI_DISP_H_