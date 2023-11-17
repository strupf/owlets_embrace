// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TRANSITION_H
#define TRANSITION_H

#include "gamedef.h"

typedef struct {
    char    to_load[64];
    int     phase;
    int     tick;
    int     hero_dir;
    int     hero_face;
    v2_i32  hero_v;
    rec_i32 heroaabb;
} transition_s;

void   transition_start(transition_s *t, const char *file);
void   transition_update(game_s *g, transition_s *t);
bool32 transition_finished(transition_s *t);
void   transition_draw(transition_s *t);

#endif