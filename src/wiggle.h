// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef WIGGLE_H
#define WIGGLE_H

#include "gamedef.h"

#define NUM_GRASS          256
#define NUM_WIGGLE         64
#define NUM_DECO_VERLET_PT 16
#define NUM_DECO_VERLET    64

typedef struct {
    v2_i16 p;  // in q6, relative to origin
    v2_i16 pp; // in q6, relative to origin
} deco_verlet_pt_s;

typedef struct {
    u8               n_pt;
    u8               n_it;
    u16              dist;
    v2_i16           grav;
    v2_i32           pos;
    bool32           haspos_2;
    v2_i16           pos_2;
    deco_verlet_pt_s pt[NUM_DECO_VERLET_PT]; // in q6, relative to origin
} deco_verlet_s;

typedef struct {
    v2_i32 pos;
    i32    type;
    i32    x_q8;
    i32    v_q8;
} grass_s;

void grass_put(game_s *g, i32 tx, i32 ty);
void grass_animate(game_s *g);
void grass_draw(game_s *g, rec_i32 camrec, v2_i32 camoffset);
//
void deco_verlet_animate(game_s *g);
void deco_verlet_draw(game_s *g, v2_i32 cam);
#endif