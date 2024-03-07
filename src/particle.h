// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PARTICLE_H
#define PARTICLE_H

#include "gamedef.h"

#define PARTICLE_NUM 256

enum {
    PARTICLE_GFX_REC,
    PARTICLE_GFX_CIR,
    PARTICLE_GFX_SPR
};

typedef struct {
    v2_i32   p_q8;
    v2_i32   v_q8;
    v2_i32   a_q8;
    i32      ticks;
    i32      ticks_max;
    i32      size;
    i32      gfx;
    texrec_s texrec;
} particle_s;

typedef struct {
    particle_s p;

    v2_i32 pr_q8;
    v2_i32 vr_q8;
    v2_i32 ar_q8;
    i32    ticksr;
    i32    sizer;
} particle_desc_s;

typedef struct {
    i32        n;
    particle_s particles[PARTICLE_NUM];
} particles_s;

void particles_spawn(game_s *g, particles_s *pr, particle_desc_s desc, int n);
void particles_update(game_s *g, particles_s *pr);

#endif