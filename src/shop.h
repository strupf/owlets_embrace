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
    int  ID;
    int  cost;
    int  count;
    char name[32];
} shopitem_s;

typedef struct {
    int        active;
    int        fade_in;
    int        fade_out;
    int        selected;
    int        shows_i1;
    int        shows_i2;
    int        show_interpolator_q8;
    int        n_items;
    int        buyticks;
    int        buycount;
    shopitem_s items[64];
} shop_s;

bool32 shop_active(game_s *g);
void   shop_open(game_s *g);
void   shop_update(game_s *g);
void   shop_draw(game_s *g);

#endif