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
#include "gfx.h"

bool gfx_bounds_add_point(gfx_rect* bounds, gfx_point* p) {
    bool expanded = false;
    int smx, smy, lgx, lgy;

    gfx_rect_normalize(bounds);
    smx = _min(bounds->p1.x, p->x);
    smy = _min(bounds->p1.y, p->y);
    lgx = _max(bounds->p2.x, p->x);
    lgy = _max(bounds->p2.y, p->y);
    expanded = (smx != bounds->p1.x || smy != bounds->p1.y || lgx != bounds->p2.x || lgy != bounds->p2.y);
    bounds->p1.x = smx;
    bounds->p1.y = smy;
    bounds->p2.x = lgx;
    bounds->p2.y = lgy;

    return (expanded);
}

void gfx_rect_normalize(gfx_rect* rect) {
    int smx, smy, lgx, lgy;

    // Find the min and max points
    smx = _min(rect->p1.x, rect->p2.x);
    smy = _min(rect->p1.y, rect->p2.y);
    lgx = _max(rect->p1.x, rect->p2.x);
    lgy = _max(rect->p1.y, rect->p2.y);
    // Update the rectangle with the new points
    rect->p1.x = smx;
    rect->p1.y = smy;
    rect->p2.x = lgx;
    rect->p2.y = lgy;
}
