// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void boss_init(g_s *g, boss_s *b, i32 type)
{
    b->type = type;

    switch (type) {
    case BOSS_GOLEM: {
        boss_golem_init(g, &b->golem);
        break;
    }
    }
}

void boss_update(g_s *g, boss_s *b)
{
    switch (b->type) {
    case BOSS_GOLEM: {
        boss_golem_update(g, &b->golem);
        break;
    }
    }
}

void boss_draw(g_s *g, boss_s *b, v2_i32 cam)
{
    switch (b->type) {
    case BOSS_GOLEM: {
        boss_golem_draw(g, &b->golem, cam);
        break;
    }
    }
}