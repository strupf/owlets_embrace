// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ITEM_SELECT_H
#define ITEM_SELECT_H

#include "core/inp.h"
#include "gamedef.h"

#define ITEM_SELECT_SCROLL_TICK 15

typedef struct item_select_s {
    bool16            docked; // connection to crank
    i16               n_items;
    i16               item;
    u16               ang_q16;
    i16               tick_item_scroll;
    inp_crank_click_s crank_click;
} item_select_s;

void item_select_init(item_select_s *i);
void item_select_update(item_select_s *i);
void item_select_draw(item_select_s *i);
i32  item_select_clutch_dist_x_q16(item_select_s *i);

#endif
