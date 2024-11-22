/**
 * Copyright 2023-24 AESilky
 *
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _DISPLAY_ILI_H_
#define _DISPLAY_ILI_H_
#ifdef __cplusplus
 extern "C" {
#endif

#include "../display.h"

/**
 * @brief Fill an RGB18 buffer with an RGB18 value.
 * @ingroup display
 *
 * @param rgb_buf The buffer to fill
 * @param rgb The value to fill with
 * @param size The size of the buffer (in rgb18_t's)
 */
extern void rgb18_buf_fill(rgb18_t* rgb_buf, rgb18_t rgb, size_t size);

#ifdef __cplusplus
}
#endif
#endif // _DISPLAY_ILI_H_

