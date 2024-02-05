// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef COLLECTIBLE_H
#define COLLECTIBLE_H

#include "gamedef.h"

#define NUM_COLLECTIBLE            256
#define COLLECTIBLE_COLLECT_DISTSQ 300

typedef struct {
    u16    type;
    u16    amount;
    i32    tick;
    v2_i32 pos;
    v2_i16 pos_q8;
    v2_i16 vel_q8;
    v2_i16 acc_q8;
    v2_i16 drag_q8;
} collectible_s;

collectible_s *collectible_create(game_s *g);
void           collectibles_update(game_s *g);
void           collectibles_draw(game_s *g, v2_i32 cam);

#endif