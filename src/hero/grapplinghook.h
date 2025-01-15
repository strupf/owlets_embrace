// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GRAPPLINGHOOK_H
#define GRAPPLINGHOOK_H

#include "gamedef.h"
#include "rope.h"

enum {
    GRAPPLINGHOOK_INACTICE,
    GRAPPLINGHOOK_FLYING,
    GRAPPLINGHOOK_HOOKED_SOLID,
    GRAPPLINGHOOK_HOOKED_TERRAIN,
    GRAPPLINGHOOK_HOOKED_OBJ,
};
#define HERO_ROPE_LEN_MIN             500
#define HERO_ROPE_LEN_MIN_JUST_HOOKED 1000
#define HERO_ROPE_LEN_SHORT           3000
#define HERO_ROPE_LEN_LONG            5000
#define GHOOK_N_HIST                  4

typedef struct grapplinghook_s {
    i32          state;
    i32          n_ang;
    v2_f32       anghist[GHOOK_N_HIST];
    v2_i32       p;
    v2_i16       p_q8;
    v2_i16       v_q8;
    obj_handle_s o1;
    obj_handle_s o2;
    ropenode_s  *rn; // in case of no hooked obj
    rope_s       rope;
} grapplinghook_s;

void   grapplinghook_create(g_s *g, grapplinghook_s *h, obj_s *ohero,
                            v2_i32 p, v2_i32 v);
void   grapplinghook_destroy(g_s *g, grapplinghook_s *h);
void   grapplinghook_update(g_s *g, grapplinghook_s *h);
bool32 grapplinghook_rope_intact(g_s *g, grapplinghook_s *h);
void   grapplinghook_animate(g_s *g, grapplinghook_s *h);
void   grapplinghook_draw(g_s *g, grapplinghook_s *h, v2_i32 cam);

// grapplinghook at offset sx/sy
bool32 grapplinghook_try_grab_obj(g_s *g, grapplinghook_s *h,
                                  obj_s *o, i32 sx, i32 sy);
// grapplinghook at offset sx/sy
bool32 grapplinghook_try_grab_terrain(g_s *g, grapplinghook_s *h,
                                      i32 sx, i32 sy);

v2_i32 rope_recalc_v(g_s *g, rope_s *r, ropenode_s *rn,
                     v2_i32 subpos_q8, v2_i32 v_q8);
v2_i32 grapplinghook_vlaunch_from_angle(i32 a_q16, i32 vel);

#endif