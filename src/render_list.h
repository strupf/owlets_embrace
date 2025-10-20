// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// maybe?

#ifndef RENDER_LIST_H
#define RENDER_LIST_H

#include "gamedef.h"

#define RENDER_LIST_MAX_ITEMS 256

typedef void (*render_item_f)(gfx_ctx_s ctx_display, g_s *g, v2_i32 cam, void *arg);

typedef struct {
    ALIGNAS(16)
    i32           priority;
    u32           UID;
    void         *arg;
    render_item_f f;
} render_item_s;

typedef struct {
    u32 UID;

    ALIGNAS(32)
    bool32        needs_sorting;
    i32           n_items;
    render_item_s items[RENDER_LIST_MAX_ITEMS];
} render_list_s;

// adds the render function to the queue and returns a unique ID to refer to this item
u32 render_list_add(render_list_s *list, render_item_f f, void *arg, i32 priority);

// delete an item with the UID; returns true if the item was found and deleted
bool32 render_list_del(render_list_s *list, u32 UID);

// delete multiple items; returns number of items deleted
i32 render_list_del_arr(render_list_s *list, u32 *UIDs, i32 n_UIDs);

// clear the render list from all items
void render_list_clr(render_list_s *list);

// run the render list; sorts all items if needed
void render_list_execute(gfx_ctx_s ctx, g_s *g, render_list_s *list);
#endif