/**
 * @brief Coordinate Geometry functionality.
 * @ingroup cg
 *
 * Provides Coordinate Geometry primatives and operations.
 *
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _COGEO_H_
#define _COGEO_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct _cg_point_ {
    int x;
    int y;
} cg_point;

typedef struct _cg_rect_ {
    cg_point p1;
    cg_point p2;
} cg_rect;

/**
 * @brief Order the corner points such that `p1` is to the upper left.
 * @ingroup cg
 *
 * @param rect Pointer to the rectangle to normalize.
 */
extern void cg_rect_normalize(cg_rect *rect);

#ifdef __cplusplus
    }
#endif
#endif // _COGEO_H_
