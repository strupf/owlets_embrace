// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void boss_init(g_s *g, i32 type)
{
    boss_s *b = &g->boss;
    b->type   = type;

    switch (type) {
    case BOSS_ID_PLANT: {
        boss_a_load(g, &b->boss_a);
        break;
    }
    }
}

void boss_update(g_s *g)
{
    boss_s *b = &g->boss;

    switch (b->type) {
    case BOSS_ID_PLANT: {
        boss_a_update(g, &b->boss_a);
        break;
    }
    }
}

void boss_animate(g_s *g)
{
    boss_s *b = &g->boss;

    switch (b->type) {
    case BOSS_ID_PLANT: {
        boss_a_animate(g, &b->boss_a);
        break;
    }
    }
}

void boss_draw(g_s *g, v2_i32 cam)
{
    boss_s *b = &g->boss;

    switch (b->type) {
    case BOSS_ID_PLANT: {
        boss_a_draw(g, &b->boss_a, cam);
        break;
    }
    }
}

void boss_draw_post(g_s *g, v2_i32 cam)
{
    boss_s *b = &g->boss;

    switch (b->type) {
    case BOSS_ID_PLANT: {
        boss_a_draw_post(g, &b->boss_a, cam);
        break;
    }
    }
}