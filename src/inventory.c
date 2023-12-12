// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "inventory.h"
#include "game.h"

const inventory_item_desc_s g_item_desc[INVENTORY_NUM_ITEMS] =
    {
        {0}};

static int inventory_find(inventory_s *i, int ID)
{
    for (int n = 0; n < i->n_items; n++) {
        if (i->items[n].ID == ID) return n;
    }
    return -1;
}

void inventory_add(inventory_s *i, int ID, int count)
{
    int k = inventory_find(i, ID);
    if (k >= 0) {
        i->items[k].count += count;
    } else {
        inventory_item_s *item = &i->items[i->n_items++];
        item->ID               = ID;
        item->count            = count;
    }
}

void inventory_rem(inventory_s *i, int ID, int count)
{
    int k = inventory_find(i, ID);
    if (k < 0) return;
    i->items[k].count -= count;
    if (i->items[k].count <= 0) {
        i->items[k] = i->items[--i->n_items];
    }
}

int inventory_count_of(inventory_s *i, int ID)
{
    int k = inventory_find(i, ID);
    return (k >= 0 ? i->items[k].count : 0);
}