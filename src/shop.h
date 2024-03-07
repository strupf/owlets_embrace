// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SHOP_H
#define SHOP_H

#include "gamedef.h"

enum {
    SHOP_ITEM_X,
};

typedef struct {
    i32  ID;
    i32  cost;
    i32  count;
    char name[32];
} shopitem_s;

typedef struct {
    i32        active;
    i32        fade_in;
    i32        fade_out;
    i32        selected;
    i32        shows_i1;
    i32        shows_i2;
    i32        show_interpolator_q8;
    i32        n_items;
    i32        buyticks;
    i32        buycount;
    shopitem_s items[64];
} shop_s;

bool32 shop_active(game_s *g);
void   shop_open(game_s *g);
void   shop_update(game_s *g);
void   shop_draw(game_s *g);

#endif