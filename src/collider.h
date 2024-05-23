// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// potential solid-actor movement rework

#ifndef COLLIDER_H
#define COLLIDER_H

#include "gamedef.h"

enum {
    COLLIDER_COLLIDE_MAP  = 1 << 0,
    COLLIDER_SIDESTEP     = 1 << 1, // can interact with slopes at all
    COLLIDER_CLIMB_SLOPES = 1 << 2, // for climbing slopes upwards
    COLLIDER_GLUE_GROUND  = 1 << 3,
};

enum {
    COLLIDER_BUMP_X_POS = 1 << 0,
    COLLIDER_BUMP_X_NEG = 1 << 1,
    COLLIDER_BUMP_Y_POS = 1 << 2,
    COLLIDER_BUMP_Y_NEG = 1 << 3,
    COLLIDER_BUMP_X     = (COLLIDER_BUMP_X_POS | COLLIDER_BUMP_X_NEG),
    COLLIDER_BUMP_Y     = (COLLIDER_BUMP_Y_POS | COLLIDER_BUMP_Y_NEG),
    COLLIDER_BUMP       = (COLLIDER_BUMP_X | COLLIDER_BUMP_Y),
};

typedef struct {
    rec_i32 r;
    i32     m;
    flags32 f;
    flags32 bump;
} collider_s;

typedef struct {
    i32        n_colls;
    collider_s colls[64];
    u8         tiles[8 * 8];
    i32        tiles_x;
    i32        tiles_y;
    i32        pixel_x;
    i32        pixel_y;
} collider_game_s;

bool32 map_solid(collider_game_s *g, collider_s *c, rec_i32 r, i32 m);
i32    collider_move_x(collider_game_s *g, collider_s *c, i32 dx, bool32 slide, i32 mpush);
i32    collider_move_y(collider_game_s *g, collider_s *c, i32 dy, bool32 slide, i32 mpush);

#endif