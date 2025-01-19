/**
 * @brief Graphics functionality.
 * @ingroup gfx
 *
 * Provides graphics primatives and operations. It is independent of the display.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _GFX_H_
#define _GFX_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Text line & column position.
 * @ingroup gfx
 */
typedef struct _text_position_ {
    uint16_t line;
    uint16_t column;
} text_position_t;


typedef struct _gfx_point_ {
    int x;
    int y;
} gfx_point;

typedef struct _gfx_rect_ {
    gfx_point p1;
    gfx_point p2;
} gfx_rect;

static inline int _max(int a, int b) { return (a > b ? a : b); }
static inline int _min(int a, int b) { return (a < b ? a : b); }

/**
 * @brief Incorporate a point into a bounding rectangle.
 * @ingroup gfx
 *
 * Increase the bounding rectangle as needed to include the point.
 *
 * @param rect The bounding rectangle (start with a zero size rectangle)
 * @param point The point to incorporate
 * @return true If the bounds were expanded
 * @return false If the point was within the existing bounds
 */
extern bool gfx_bounds_add_point(gfx_rect *bounds, gfx_point *p);

/**
 * @brief Order the corner points such that `p1` is to the upper left.
 * @ingroup gfx
 *
 * @param rect Pointer to the rectangle to normalize.
 */
extern void gfx_rect_normalize(gfx_rect *rect);

#ifdef __cplusplus
    }
#endif
#endif // _GFX_H_
