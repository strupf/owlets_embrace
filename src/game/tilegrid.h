// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TILEGRID_H
#define TILEGRID_H

#include "gamedef.h"

// we operate on 16x16 tiles
struct tilegrid_s {
        u8 *tiles; // tileIDs
        int tiles_x;
        int tiles_y;
        int pixel_x;
        int pixel_y;
};

extern const tri_i32 tilecolliders[];

bool32     tiles_area(tilegrid_s tg, rec_i32 r);
bool32     tiles_at(tilegrid_s tg, i32 x, i32 y);
tilegrid_s tilegrid_from_game(game_s *g);
void       tilegrid_bounds_minmax(game_s *g, v2_i32 pmin, v2_i32 pmax,
                                  i32 *x1, i32 *y1, i32 *x2, i32 *y2);
void       tilegrid_bounds_tri(game_s *g, tri_i32 t, i32 *x1, i32 *y1, i32 *x2, i32 *y2);
void       tilegrid_bounds_rec(game_s *g, rec_i32 r, i32 *x1, i32 *y1, i32 *x2, i32 *y2);

#endif