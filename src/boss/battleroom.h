// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BATTLEROOM_H
#define BATTLEROOM_H

#include "gamedef.h"
#include "map_loader.h"

enum {
    BATTLEROOM_NONE,
    BATTLEROOM_IDLE,
    BATTLEROOM_STARTING,
    BATTLEROOM_ACTIVE,
    BATTLEROOM_NEXT_PHASE,
    BATTLEROOM_ENDING,
    BATTLEROOM_ENDING_2
};

typedef struct {
    ALIGNAS(32)
    rec_i32 r;
    u32     n_killed_prior;
    u16     saveID;
    u16     timer;
    u8      state;
    u8      phase;
    u8      n_enemies;
} battleroom_s;

void battleroom_load(g_s *g, map_obj_s *mo);
void battleroom_on_update(g_s *g);

#endif