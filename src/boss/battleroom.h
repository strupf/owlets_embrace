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

typedef struct battleroom_s {
    ALIGNAS(32)
    rec_i32 r;
    u32     n_killed_prior;
    u16     saveID;
    u16     timer;
    u16     trigger_activate;
    u16     trigger_finish;
    u8      ID;
    u8      state;
    u8      phase;
    u8      n_enemies;
    u8      cam_rec_ID;
    u8      ticks_to_spawn;
    u8      n_map_obj_to_spawn;
    void   *map_obj_to_spawn[16];
} battleroom_s;

void battleroom_load(g_s *g, map_obj_s *mo);
void battleroom_on_update(g_s *g);

#endif