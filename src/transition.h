// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TRANSITION_H
#define TRANSITION_H

#include "gamedef.h"

typedef struct transition_s transition_s;

struct transition_s {
    int    fade_phase;
    int    fade_tick;
    int    type;
    int    hero_face;
    v2_i32 hero_v;
    v2_i32 hero_feet;
    char   to_load[64];
    int    dir;
};

void   transition_teleport(transition_s *t, game_s *g, const char *mapfile, v2_i32 hero_feet);
void   transition_check_herodata_slide(transition_s *t, game_s *g);
void   transition_update(game_s *g, transition_s *t);
bool32 transition_blocks_gameplay(transition_s *t);
bool32 transition_finished(transition_s *t);
void   transition_draw(game_s *g, transition_s *t, v2_i32 camoffset);

#endif