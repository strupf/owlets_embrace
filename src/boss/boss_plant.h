// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BOSS_PLANT_H
#define BOSS_PLANT_H

#include "gamedef.h"

#define BOSS_PLANT_INTRO_TICKS 50

enum {
    BOSS_PLANT_INTRO,
    BOSS_PLANT_IDLE,
    BOSS_PLANT_DYING,
};

typedef struct {
    i32 tick;
    i32 phase;
} boss_plant_s;

void boss_plant_load(g_s *g);
void boss_plant_update(g_s *g, boss_plant_s *b);
void boss_plant_draw(g_s *g, boss_plant_s *b, v2_i32 cam);

#endif