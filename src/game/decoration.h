// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef DECORATION_H
#define DECORATION_H

#include "game_def.h"

enum {
        BACKGROUND_WIND_CIRCLE_R = 1900,
};

enum {
        BG_NUM_CLOUDS      = 64,
        BG_NUM_PARTICLES   = 256,
        BG_NUM_CLOUD_TYPES = 3,
        BG_WIND_PARTICLE_N = 8,
};

typedef struct {
        int    cloudtype;
        v2_i32 p; // q8
        i32    v; // q8
} cloudbg_s;

typedef struct {
        int    n;
        v2_i32 p;
        v2_i32 v;
        v2_i32 pos[BG_WIND_PARTICLE_N];
        v2_i32 circc;
        i32    ticks;
        i32    circticks;
        i32    circcooldown;
} particlebg_s;

typedef struct {
        v2_i32 p_q8;
        v2_i32 v_q8;
        v2_i32 a_q8;
        i32    ticks;
} particle_s;

void decoration_setup(game_s *g);
void room_deco_animate(game_s *g);

#endif