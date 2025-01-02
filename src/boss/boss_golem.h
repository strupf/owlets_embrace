// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BOSS_GOLEM_H
#define BOSS_GOLEM_H

#include "gamedef.h"
#include "tile_map.h"

enum {
    BOSS_GOLEM_P_INTRO,
    BOSS_GOLEM_P_IDLE,
    BOSS_GOLEM_P_SLAM_CRACKING,
    BOSS_GOLEM_P_SLAM,
};

#define BOSS_GOLEM_NUM_TILES 1024

typedef struct {
    i32    n_platforms;
    obj_s *platforms[16];
    obj_s *golem;

    i32 health;
    i32 phase;
    i32 phase_tick;
    i32 phase_tick_end;

    // block of terrain tiles
    // copy pasted
    i32    tx;
    i32    ty;
    i32    tw;
    i32    th;
    tile_s tiles[BOSS_GOLEM_NUM_TILES];
} boss_golem_s;

void boss_golem_init(g_s *g, boss_golem_s *b);
void boss_golem_update(g_s *g, boss_golem_s *b);
void boss_golem_draw(g_s *g, boss_golem_s *b, v2_i32 cam);

#endif