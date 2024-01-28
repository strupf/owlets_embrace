// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SHOP_H
#define SHOP_H

#include "gamedef.h"

typedef struct {
    int active;

} shop_s;

bool32 shop_active(game_s *g);
void   shop_open(game_s *g);
void   shop_update(game_s *g);
void   shop_draw(game_s *g);

#endif