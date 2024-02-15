// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef INVENTORY_H
#define INVENTORY_H

#define INVENTORY_NUM_ITEMS 256

#include "gamedef.h"

enum {
    INVENTORY_ID_GOLD,
    INVENTORY_ID_KEY_CIRCLE,
    INVENTORY_ID_KEY_SQUARE,
    INVENTORY_ID_KEY_TRIANGLE,
};

typedef struct {
    char name[64];
    char desc[256];
} inventory_item_desc_s;

typedef struct {
    int ID;
    int n;
} inventory_item_s;

typedef struct {
    int              n_items;
    inventory_item_s items[256];
} inventory_s;

extern const inventory_item_desc_s g_item_desc[INVENTORY_NUM_ITEMS];

void inventory_add(inventory_s *i, int ID, int n);
void inventory_rem(inventory_s *i, int ID, int n);
int  inventory_count_of(inventory_s *i, int ID);

#endif