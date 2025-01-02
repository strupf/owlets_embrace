// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BOSS_H
#define BOSS_H

#include "boss_golem.h"
#include "gamedef.h"

enum {
    BOSS_NONE,
    BOSS_GOLEM,
};

typedef struct {
    i32 type;
    union {
        boss_golem_s golem;
    };
} boss_s;

void boss_init(g_s *g, boss_s *b, i32 type);
void boss_update(g_s *g, boss_s *b);
void boss_draw(g_s *g, boss_s *b, v2_i32 cam);

#endif