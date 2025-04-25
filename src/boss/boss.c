// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void boss_init(g_s *g, i32 type, map_obj_s *mo)
{
    boss_s *b = &g->boss;
    b->type   = type;

    switch (type) {
    case BOSS_ID_PLANT: {
        boss_plant_load(g, mo);
        break;
    }
    }
}

void boss_update(g_s *g)
{
    boss_s *b = &g->boss;

    switch (b->type) {
    case BOSS_ID_PLANT: {
        boss_plant_update(g);
        break;
    }
    }
}

void boss_draw(g_s *g, v2_i32 cam)
{
    boss_s *b = &g->boss;

    switch (b->type) {
    case BOSS_ID_PLANT: {
        boss_plant_draw(g, cam);
        break;
    }
    }
}

void boss_draw_post(g_s *g, v2_i32 cam)
{
    boss_s *b = &g->boss;

    switch (b->type) {
    case BOSS_ID_PLANT: {
        boss_plant_draw_post(g, cam);
        break;
    }
    }
}