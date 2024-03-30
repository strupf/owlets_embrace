// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _HEROUPGRADE_H
#define _HEROUPGRADE_H

#include "gamedef.h"

typedef struct {
    i32 phase;
    i32 t;
    i32 upgrade;
} heroupgrade_s;

void heroupgrade_update(game_s *g);
void heroupgrade_draw(game_s *g, v2_i32 cam);

#endif