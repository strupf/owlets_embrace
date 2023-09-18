// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BACKFOREGROUND_H
#define BACKFOREGROUND_H

#include "gamedef.h"

enum {
        BACKGROUND_WIND_CIRCLE_R = 1900,
};

enum {
        BG_NUM_CLOUDS      = 32,
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
        cloudbg_s    clouds[BG_NUM_CLOUDS];
        int          nclouds;
        particlebg_s particles[BG_NUM_PARTICLES];
        int          nparticles;
        int          clouddirection;
} backforeground_s;

void backforeground_setup(game_s *g);
void backforeground_animate(game_s *g);
#endif