// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _MAPTRANSITION_H
#define _MAPTRANSITION_H

#include "gamedef.h"

typedef struct maptransition_s {
    u8     fade_phase;
    u8     type;
    u8     health;
    i32    hero_face;
    bool8  jump_ui_may_hide;
    i32    dir;
    u16    jump_ui_tick;
    i16    fade_tick;
    u16    stamina;
    v2_i16 hero_v;
    v2_i32 hero_feet;
    i32    teleport_ID;
    char   to_load[64];
} maptransition_s;

void   maptransition_teleport(g_s *g, const char *map, v2_i32 hero_feet);
bool32 maptransition_try_hero_slide(g_s *g);
void   maptransition_update(g_s *g);
void   maptransition_draw(g_s *g, v2_i32 cam);

#endif