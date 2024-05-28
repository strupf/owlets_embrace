// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef WIGGLE_H
#define WIGGLE_H

#include "gamedef.h"

#define NUM_GRASS  256
#define NUM_WIGGLE 64

typedef struct {
    v2_i32 pos;
    i32    type;
    i32    x_q8;
    i32    v_q8;
} grass_s;

typedef struct {
    texrec_s tr;
    rec_i32  r;
    i32      t;
    v2_i32   offs;
    bool32   overlaps;
} wiggle_deco_s;

void grass_put(game_s *g, i32 tx, i32 ty);
void grass_animate(game_s *g);
void grass_draw(game_s *g, rec_i32 camrec, v2_i32 camoffset);

void wiggle_add(game_s *g, map_obj_s *mo);
void wiggle_animate(game_s *g);
void wiggle_draw(game_s *g, v2_i32 camoffset);

#endif