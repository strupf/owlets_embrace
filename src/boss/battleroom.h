// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BATTLEROOM_H
#define BATTLEROOM_H

#include "gamedef.h"

enum {
    BATTLEROOM_NONE,
    BATTLEROOM_IDLE,
    BATTLEROOM_STARTING,
    BATTLEROOM_ACTIVE,
    BATTLEROOM_ENDING
};

typedef struct {
    u32     n_killed_prior;
    i32     saveID;
    void   *mem_enemies;
    u16     timer;
    u8      state;
    u8      n_enemies;
    bool32  has_camfocus;
    v2_i32  camfocus;
    rec_i32 r;
} battleroom_s;

void battleroom_on_update(g_s *g);

#endif