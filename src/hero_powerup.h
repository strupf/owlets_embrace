// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
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

void hero_powerup_collected(g_s *g, i32 ID);
void hero_powerup_update(g_s *g);
void hero_powerup_draw(g_s *g, v2_i32 cam);

#endif