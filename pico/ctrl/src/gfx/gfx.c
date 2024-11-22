/**
 * @brief Graphics functionality.
 * @ingroup gfx
 *
 * Provides graphics primatives and operations. It is independent of the display.
 *
 * Copyright 2023-24 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "gfx.h"

static inline int _max(int a, int b) {return (a > b ? a : b);}
static inline int _min(int a, int b) {return (a < b ? a : b);}

void gfx_rect_normalize(gfx_rect* rect) {
    int smx, smy, lgx, lgy;

    // Find the min and max points
    smx = _min(rect->p1.x, rect->p2.x);
    smy = _min(rect->p1.y, rect->p2.y);
    lgx = _max(rect->p1.x, rect->p2.x);
    lgy = _max(rect->p1.y, rect->p2.y);
    // Update the rectange with the new points
    rect->p1.x = smx;
    rect->p1.y = smy;
    rect->p2.x = lgx;
    rect->p2.y = lgy;
}
