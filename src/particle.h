// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
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

enum {
    PARTICLE_TYPE_CIR,
    PARTICLE_TYPE_TEX,
};

enum {
    PARTICLE_FLAG_COLLISIONS,
};

typedef struct {
    u8     typ;
    u8     cir_r;
    u8     cir_r_range;
    u8     cir_col;
    v2_i16 pos;
    u8     pos_x_range;
    u8     pos_y_range;
    u8     acc_ang;
    u8     acc_ang_range;
    u8     acc;
    u8     acc_range;
    u8     vel_ang;
    u8     vel_ang_range;
    u16    vel;
    u16    vel_range;
    u16    tex_frames;
    u16    texID;
    u16    tex_x;
    u16    tex_y;
    u16    tex_w;
    u16    tex_h;
    u16    tic;
    u16    tic_range;
} particle_emitter_s;

typedef struct {
    u8     typ;
    u8     cir_r;
    u8     cir_col;
    v2_i16 pos;
    v2_i16 subpos;
    v2_i16 vel;
    v2_i16 acc;
    u16    texID;
    u16    tex_x;
    u16    tex_y;
    u16    tex_w;
    u16    tex_h;
    u16    tic;
    u16    tic_max;
} particle_2_s;

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

void particles_spawn(g_s *g, particle_desc_s desc, i32 n);
void particles_update(g_s *g, particles_s *pr);
void particles_draw(g_s *g, particles_s *pr, v2_i32 cam);

#endif