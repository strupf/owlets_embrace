// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BOSS_H
#define BOSS_H

#include "boss/boss_a.h"
#include "gamedef.h"

enum {
    BOSS_ID_NONE,
    BOSS_ID_PLANT,
};

typedef struct {
    i32 type;
    union {
        boss_a_s boss_a;
    };

    void *heap;
} boss_s;

void boss_init(g_s *g, i32 type);
void boss_update(g_s *g);
void boss_animate(g_s *g);
void boss_draw(g_s *g, v2_i32 cam);
void boss_draw_post(g_s *g, v2_i32 cam);
void boss_on_trigger(g_s *g, i32 trigger);

#endif