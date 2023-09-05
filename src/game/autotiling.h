// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef AUTOTILING_H
#define AUTOTILING_H

#include "game.h"

enum {
        TMJ_TILESET_FGID = NUM_AUTOTILE_TYPES * 16 + 1, // 16 in a row per tiletype
};

bool32 is_autotile(u32 tileID);
void   autotile_calc(game_s *g, u32 *arr, int w, int h,
                     int x, int y, int n, int layerID);

#endif