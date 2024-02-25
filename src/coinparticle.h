// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef COINPARTICLE_H
#define COINPARTICLE_H

#include "gamedef.h"

#define NUM_COINPARTICLE            256
#define COINPARTICLE_COLLECT_DISTSQ 350

typedef struct {
    u16    type;
    u16    amount;
    i32    tick;
    v2_i32 pos;
    v2_i16 pos_q8;
    v2_i16 vel_q8;
    v2_i16 acc_q8;
    v2_i16 drag_q8;
} coinparticle_s;

coinparticle_s *coinparticle_create(game_s *g);
void            coinparticle_update(game_s *g);
void            coinparticle_draw(game_s *g, v2_i32 cam);

#endif