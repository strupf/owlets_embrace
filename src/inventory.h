// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _INVENTORY_H
#define _INVENTORY_H

#include "gamedef.h"

typedef struct {
    bool32 active;
} inventory_s;

bool32 inventory_active(inventory_s *inv);
void   inventory_update(game_s *g, inventory_s *inv);
void   inventory_draw(game_s *g, inventory_s *inv);
void   inventory_open(game_s *g, inventory_s *inv);

#endif