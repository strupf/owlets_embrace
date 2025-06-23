// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GRAPPLINGHOOK_H
#define GRAPPLINGHOOK_H

#include "gamedef.h"
#include "rope.h"

typedef struct {
    i32    m_q8;
    v2_i16 v_q8;
    v2_i16 a_q8;
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

typedef struct grapplinghook_s {
    i32             state;
    i32             n_ang;
    i32             destroy_tick;
    i32             throw_snd_iID;
    ropeobj_param_s param1;
    ropeobj_param_s param2;
    i32             f_cache_dt;
    i32             f_cache_o1;
    i32             f_cache_o2;
    i32             destroy_tick_q8;
    v2_f32          anghist[GHOOK_N_HIST];
    v2_i32          p;
    v2_i16          p_q8;
    v2_i16          v_q8;
    obj_handle_s    o1;
    obj_handle_s    o2;
    ropenode_s     *rn; // in case of no hooked obj
    rope_s          rope;
} grapplinghook_s;

void   grapplinghook_create(g_s *g, grapplinghook_s *h, obj_s *ohero,
                            v2_i32 p, v2_i32 v);
void   grapplinghook_destroy(g_s *g, grapplinghook_s *h);
void   grapplinghook_update(g_s *g, grapplinghook_s *h);
bool32 grapplinghook_rope_intact(g_s *g, grapplinghook_s *h);
void   grapplinghook_animate(g_s *g, grapplinghook_s *h);
void   grapplinghook_draw(g_s *g, grapplinghook_s *h, v2_i32 cam);
void   grapplinghook_calc_f_internal(g_s *g, grapplinghook_s *h);
i32    grapplinghook_f_at_obj_proj(grapplinghook_s *gh, obj_s *o, v2_i32 dproj);
i32    grapplinghook_f_at_obj_proj_v(grapplinghook_s *gh, obj_s *o, v2_i32 dproj, v2_i32 *f_out);

// grapplinghook at offset sx/sy
bool32 grapplinghook_try_grab_obj(g_s *g, grapplinghook_s *h,
                                  obj_s *o, i32 sx, i32 sy);
// grapplinghook at offset sx/sy
bool32 grapplinghook_try_grab_terrain(g_s *g, grapplinghook_s *h,
                                      i32 sx, i32 sy);

i32    grapplinghook_pulling_force_hero(g_s *g);
v2_i32 rope_recalc_v(g_s *g, rope_s *r, ropenode_s *rn,
                     v2_i32 subpos_q8, v2_i32 v_q8);
v2_i32 grapplinghook_vlaunch_from_angle(i32 a_q16, i32 vel);
void   hero_hook_preview_throw(g_s *g, obj_s *ohero, v2_i32 cam);
#endif