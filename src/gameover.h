// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _GAMEOVER_H
#define _GAMEOVER_H

#include "gamedef.h"

typedef struct {
    i16 tick;
    i16 phase;
} gameover_s;

void gameover_start(game_s *g);
void gameover_update(game_s *g);
void gameover_draw(game_s *g, v2_i32 cam);

#endif