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
    u8      state;
    u8      n_enemies;
    u16     timer;
    i32     saveID;
    void   *mem_enemies;
    rec_i32 r;
} battleroom_s;

void battleroom_on_update(g_s *g);

#endif