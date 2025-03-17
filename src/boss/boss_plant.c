// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "boss_plant.h"
#include "game.h"

void boss_plant_spawn_eye(g_s *g);
void boss_plant_eye_on_update(g_s *g, obj_s *o);
void boss_plant_eye_on_animate(g_s *g, obj_s *o);

void boss_plant_load(g_s *g)
{
}

void boss_plant_update(g_s *g, boss_plant_s *b)
{
    b->tick++;

    switch (b->phase) {
    case BOSS_PLANT_INTRO: {
        break;
    }
    case BOSS_PLANT_IDLE: {
        break;
    }
    }
}

void boss_plant_draw(g_s *g, boss_plant_s *b, v2_i32 cam)
{
    switch (b->phase) {
    case BOSS_PLANT_INTRO: {
        break;
    }
    case BOSS_PLANT_IDLE: {
        break;
    }
    }
}

void boss_plant_spawn_eye(g_s *g)
{
    obj_s *o      = obj_create(g);
    o->on_update  = boss_plant_eye_on_update;
    o->on_animate = boss_plant_eye_on_animate;
    o->w          = 16;
    o->h          = 16;
}

void boss_plant_eye_on_update(g_s *g, obj_s *o)
{
}

void boss_plant_eye_on_animate(g_s *g, obj_s *o)
{
}