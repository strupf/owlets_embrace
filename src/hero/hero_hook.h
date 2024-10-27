// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_HOOK_H
#define HERO_HOOK_H

#include "gamedef.h"
#include "rope.h"

#define HERO_ROPE_LEN_MIN             500
#define HERO_ROPE_LEN_MIN_JUST_HOOKED 1000
#define HERO_ROPE_LEN_SHORT           3000
#define HERO_ROPE_LEN_LONG            5000
#define HEROHOOK_N_HIST               4

typedef struct {
    i32    n_ang;
    v2_f32 anghist[HEROHOOK_N_HIST];
} herohook_s;

static_assert(sizeof(herohook_s) <= 256, "M");

obj_s *hook_create(game_s *g, rope_s *r, v2_i32 p, v2_i32 v_q8);
void   hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook);
i32    hook_move(game_s *g, obj_s *o, v2_i32 dt, obj_s **ohook);
void   hook_update(game_s *g, obj_s *hook); // called by hero
void   hook_on_animate(game_s *g, obj_s *o);
bool32 hook_is_attached(obj_s *o);

// returns relative stretched length in q8 > 0 if stretched
i32 hook_is_stretched(game_s *g, obj_s *o);

#endif