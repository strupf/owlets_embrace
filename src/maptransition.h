// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _MAPTRANSITION_H
#define _MAPTRANSITION_H

#include "gamedef.h"

typedef struct {
    i32    fade_phase;
    i32    fade_tick;
    i32    type;
    i32    hero_face;
    v2_i32 hero_v;
    v2_i32 hero_feet;
    i32    teleport_ID;
    char   to_load[64];
    i32    dir;
} maptransition_s;

void   maptransition_teleport(game_s *g, const char *map, v2_i32 hero_feet);
bool32 maptransition_try_hero_slide(game_s *g);
void   maptransition_update(game_s *g);
void   maptransition_draw(game_s *g, v2_i32 cam);

#endif