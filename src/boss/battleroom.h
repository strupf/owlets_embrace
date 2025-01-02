// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BATTLEROOM_H
#define BATTLEROOM_H

#include "gamedef.h"

typedef struct {
    bool32 active;
    i32    enemies_left;
    i32    trigger_on_end;
} battleroom_s;

void battleroom_on_update(g_s *g, battleroom_s *r);

#endif