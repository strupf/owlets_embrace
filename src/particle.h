// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PARTICLE_H
#define PARTICLE_H

#include "gamedef.h"

enum {
    PARTICLE_GFX_REC,
    PARTICLE_GFX_CIR,
    PARTICLE_GFX_SPR
};

enum {
    PARTICLE_TYPE_CIR,
    PARTICLE_TYPE_REC,
    PARTICLE_TYPE_TEX,
    //
    PARTICLE_NUM_TYPES
};

enum {
    PARTICLE_FLAG_COLLISIONS = 1 << 6,
    PARTICLE_FLAG_FADE_OUT   = 1 << 7,
};

#define PARTICLE_MASK_TYPE 3

typedef struct {
    u8 n_frames;
    u8 x; // multiple of 8
    u8 y; // multiple of 8
    u8 w; // multiple of 8
    u8 h; // multiple of 8
} particle_tex_s;

typedef struct {
    u8 size_beg;
    u8 size_end;
} particle_prim_s;

typedef struct {
    u8             in_use;
    u16            amount;
    u16            tick;
    u16            interval;
    obj_handle_s   o;
    // spawn data below
    u16            ticks_min;
    u16            ticks_max;
    u8             type;
    u8             mode;
    u8             size_beg_min;
    u8             size_beg_max;
    u8             size_end_min;
    u8             size_end_max;
    u8             p_range_r;
    u8             drag;
    particle_tex_s tex;
    v2_i16         p;
    v2_i16         p_range;
    v2_i16         v_q8;
    v2_i16         a_q8;
    v2_i16         v_q8_range;
    v2_i16         a_q8_range;
} particle_emit_s;

typedef struct {
    ALIGNAS(32)
    v2_i16 p;
    v2_i16 p_q8;
    v2_i16 v_q8;
    v2_i16 a_q8;
    u16    ticks;
    u16    ticks_max;
    u8     type; // type and flags
    u8     mode; // mode for prim or flipping flags
    u8     drag;
    union {
        particle_prim_s prim;
        particle_tex_s  tex;
    };
} particle_s;

static_assert(sizeof(particle_s) == 32, "size particle");

typedef struct {
    u32             seed;
    i32             n;
    particle_s      particles[512];
    particle_emit_s emitters[16];
} particle_sys_s;

particle_emit_s *particle_emitter_create(g_s *g);
void             particle_emitter_destroy(g_s             *g,
                                          particle_emit_s *pe);

void particle_emitter_emit(g_s *g, particle_emit_s *pe, i32 n);
void particle_sys_update(g_s *g);
void particle_sys_draw(g_s *g, v2_i32 cam);
void particle_sys_shuffle(particle_sys_s *pr, i32 n1, i32 n);

#endif