// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BOSS_H
#define BOSS_H

#include "boss/boss_plant.h"
#include "gamedef.h"

enum {
    BOSS_ID_NONE,
    BOSS_ID_PLANT,
};

typedef struct {
    i32 type;
    union {
        boss_plant_s plant;
    };
} boss_s;

void boss_init(g_s *g, i32 type, map_obj_s *mo);
void boss_update(g_s *g);
void boss_draw(g_s *g, v2_i32 cam);
void boss_draw_post(g_s *g, v2_i32 cam);

#endif