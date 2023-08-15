// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BACKFOREGROUND_H
#define BACKFOREGROUND_H
#include "gamedef.h"

typedef struct {
        int    cloudtype;
        v2_i32 pos;
        i32    velx;
} cloudbg_s;

typedef struct {
        int    n;
        v2_i32 p;
        v2_i32 v;
        v2_i32 pos[16];
        v2_i32 circc;
        i32    ticks;
        i32    circticks;
        i32    circcooldown;
} particlebg_s;

typedef struct {
        cloudbg_s    clouds[NUM_BACKGROUND_CLOUDS];
        int          nclouds;
        particlebg_s particles[NUM_BACKGROUND_PARTICLES];
        int          nparticles;
} backforeground_s;

void background_foreground_animate(game_s *g);
#endif