// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ITEM_SELECT_H
#define ITEM_SELECT_H

#include "gamedef.h"

typedef struct {
    bool16 docked; // connection to crank
    i16    n_items;
    i32    item;
    i32    ang_q16;
} item_select_s;

void item_select_update(item_select_s *i);
void item_select_draw(item_select_s *i);
i32  item_select_clutch_dist_x_q16(item_select_s *i);

#endif
