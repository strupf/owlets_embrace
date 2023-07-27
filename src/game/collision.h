/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#ifndef COLLISION_H
#define COLLISION_H

#include "gamedef.h"

enum {
        TILE_EMPTY,
        TILE_BLOCK,
        TILE_SLOPE_45_1,
        TILE_SLOPE_45_2,
        TILE_SLOPE_45_3,
        TILE_SLOPE_45_4,
};

// we operate on 16x16 tiles
struct tilegrid_s {
        u8 *tiles; // tileIDs
        int tiles_x;
        int tiles_y;
        int pixel_x;
        int pixel_y;
};

bool32 tiles_area(tilegrid_s tg, rec_i32 r);
bool32 tiles_at(tilegrid_s tg, i32 x, i32 y);

#endif