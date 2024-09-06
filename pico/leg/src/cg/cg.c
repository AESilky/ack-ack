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
#include "cg.h"

static inline int _max(int a, int b) {return (a > b ? a : b);}
static inline int _min(int a, int b) {return (a < b ? a : b);}

void cg_rect_normalize(cg_rect* rect) {
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
