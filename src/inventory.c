// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "inventory.h"

const inventory_item_desc_s g_item_desc[INVENTORY_NUM_ITEMS] =
    {0};

static inventory_item_s *inventory_find(inventory_s *i, int ID)
{
    for (int n = 0; n < i->n_items; n++) {
        inventory_item_s *item = &i->items[n];
        if (item->ID == ID) return item;
    }
    return NULL;
}

void inventory_add(inventory_s *i, int ID, int n)
{
    inventory_item_s *item = inventory_find(i, ID);
    if (item) {
        item->n += n;
    } else {
        item     = &i->items[i->n_items++];
        item->ID = ID;
        item->n  = n;
    }
}

void inventory_rem(inventory_s *i, int ID, int n)
{
    inventory_item_s *item = inventory_find(i, ID);
    if (!item) return;
    item->n -= min_i(n, item->n);
    if (item->n == 0) {
        *item = i->items[--i->n_items];
    }
}

int inventory_count_of(inventory_s *i, int ID)
{
    inventory_item_s *item = inventory_find(i, ID);
    return (item ? item->n : 0);
}