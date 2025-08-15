// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GRAPPLINGHOOK_H
#define GRAPPLINGHOOK_H

#include "gamedef.h"
#include "wire.h"

#define ROPE_VERLET_N 32

typedef struct {
    i32    m_q12;
    v2_i32 v_q12;
    v2_i32 a_q12;
} ropeobj_param_s;

#define ROPEOBJ_M_INF I32_MAX
#define ROPEOBJ_F_INF I32_MAX

enum {
    GRAPPLINGHOOK_INACTICE,
    GRAPPLINGHOOK_FLYING,
    GRAPPLINGHOOK_HOOKED_SOLID,
    GRAPPLINGHOOK_HOOKED_TERRAIN,
    GRAPPLINGHOOK_HOOKED_OBJ,
};

#define HERO_ROPE_LEN_MIN 500
#define HERO_ROPE_LEN_MAX 5000
#define GHOOK_N_HIST      4

typedef struct rope_pt_s {
    ALIGNAS(16)
    v2_i32 p;
    v2_i32 pp;
} rope_pt_s;

typedef struct {
    wire_s   *r;
    i32       n;
    rope_pt_s p[ROPE_VERLET_N];
} rope_verlet_s;

void rope_verlet_sim(g_s *g, rope_verlet_s *rv);

typedef struct grapplinghook_s {
    i32             state;
    i32             n_ang;
    i32             destroy_tick;
    i32             throw_snd_iID;
    u8              attached_tick;
    ropeobj_param_s param1;
    ropeobj_param_s param2;
    i32             f_cache_dt;
    i32             f_cache_o1;
    i32             f_cache_o2;
    i32             destroy_tick_q8;
    v2_i32          anghist[GHOOK_N_HIST];
    v2_i32          p_q12;
    obj_handle_s    o1;
    obj_handle_s    o2;
    obj_handle_s    solid;
    wire_s          wire;
    i32             len_max_q4;
    rope_verlet_s   rope_verlet;
} grapplinghook_s;

void   grapplinghook_create(g_s *g, grapplinghook_s *h, obj_s *ohero, v2_i32 p, v2_i32 v);
void   grapplinghook_destroy(g_s *g, grapplinghook_s *h);
void   grapplinghook_update(g_s *g, grapplinghook_s *h);
bool32 grapplinghook_rope_intact(g_s *g, grapplinghook_s *h);
void   grapplinghook_animate(g_s *g, grapplinghook_s *h);
void   grapplinghook_draw(g_s *g, grapplinghook_s *h, v2_i32 cam);
void   grapplinghook_calc_f_internal(g_s *g, grapplinghook_s *h);
i32    grapplinghook_f_at_obj_proj(grapplinghook_s *gh, obj_s *o, v2_i32 dproj);
i32    grapplinghook_f_at_obj_proj_v(grapplinghook_s *gh, obj_s *o, v2_i32 dproj, v2_i32 *f_out);
v2_i32 grapplinghook_vlaunch_from_angle(i32 a_q16, i32 vel);
v2_i32 rope_v_pulling_at_obj(wire_s *r, wirenode_s *rn, obj_s *o);
v2_i32 grapplinghook_v_damping(wire_s *r, wirenode_s *rn, v2_i32 subpos_q8, v2_i32 v_q8);
v2_i32 rope_v_to_neighbour(wire_s *r, wirenode_s *rn);
i32    grapplinghook_stretched_dt_len_abs(g_s *g, grapplinghook_s *h);

#endif