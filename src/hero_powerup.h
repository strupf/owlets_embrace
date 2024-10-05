// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef HERO_POWERUP_H
#define HERO_POWERUP_H

#include "gamedef.h"

typedef struct hero_powerup_s {
    i32 ID;
    i32 phase;
    i32 tick;
    i32 tick_total;
} hero_powerup_s;

void hero_powerup_collected(game_s *g, i32 ID);
void hero_powerup_update(game_s *g);
void hero_powerup_draw(game_s *g, v2_i32 cam);

#endif