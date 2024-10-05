// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PARTICLE_H
#define PARTICLE_H

#include "gamedef.h"

#define PARTICLE_NUM 512

enum {
    PARTICLE_GFX_REC,
    PARTICLE_GFX_CIR,
    PARTICLE_GFX_SPR
};

typedef struct {
    v2_i32   p_q8;
    v2_i32   v_q8;
    v2_i32   a_q8;
    i16      ticks;
    i16      ticks_max;
    i16      size;
    u8       gfx;
    u8       col;
    texrec_s texrec;
} particle_s;

typedef struct {
    particle_s p;

    v2_i32 pr_q8;
    v2_i32 vr_q8;
    v2_i32 ar_q8;
    i16    ticksr;
    i16    sizer;
} particle_desc_s;

typedef struct {
    i32        n;
    particle_s particles[PARTICLE_NUM];
} particles_s;

void particles_spawn(game_s *g, particle_desc_s desc, i32 n);
void particles_update(game_s *g, particles_s *pr);
void particles_draw(game_s *g, particles_s *pr, v2_i32 cam);

#define NUM_COINPARTICLE            256
#define COINPARTICLE_COLLECT_DISTSQ 350

typedef struct {
    u16    type;
    u16    amount;
    i32    tick;
    v2_i32 pos;
    v2_i16 pos_q8;
    v2_i16 vel_q8;
    v2_i16 acc_q8;
    v2_i16 drag_q8;
} coinparticle_s;

coinparticle_s *coinparticle_create(game_s *g);
void            coinparticle_update(game_s *g);
void            coinparticle_draw(game_s *g, v2_i32 cam);

#endif