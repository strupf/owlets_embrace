// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "render_list.h"
#include "game.h"

u32 render_list_add(render_list_s *list, render_item_f f, void *arg, i32 priority)
{
    if (RENDER_LIST_MAX_ITEMS <= list->n_items || !f) return 0;

    u32            UID  = ++list->UID;
    render_item_s *item = &list->items[list->n_items++];
    item->UID           = UID;
    item->f             = f;
    item->arg           = arg;
    item->priority      = priority;
    return UID;
}

bool32 render_list_del(render_list_s *list, u32 UID)
{
    for (i32 n = 0; n < list->n_items; n++) {
        render_item_s *item = &list->items[n];
        if (item->UID == UID) {
            list->n_items--;
            mmov(item, item + 1, sizeof(render_item_s) * (list->n_items - n));
            return 1;
        }
    }
    return 0;
}

i32 render_list_del_arr(render_list_s *list, u32 *UIDs, i32 n_UIDs)
{
    i32 n_del = 0;
    for (i32 n = 0; n < list->n_items; n++) {
        render_item_s *item = &list->items[n];

        for (i32 i = 0; i < n_UIDs; i++) {
            if (item->UID == UIDs[i]) {
                *item = list->items[--list->n_items];
                n_del++;
                break;
            }
        }
    }
    list->needs_sorting |= n_del;
    return n_del;
}

void render_list_clr(render_list_s *list)
{
    list->n_items       = 0;
    list->needs_sorting = 0;
    list->UID           = 0;
}

static inline i32 cmp_render_item_priority(render_item_s *a, render_item_s *b)
{
    return (i32)(a->priority) - (i32)(b->priority);
}

SORT_ARRAY_DEF(render_item_s, render_item, cmp_render_item_priority)

void render_list_execute(gfx_ctx_s ctx, g_s *g, render_list_s *list)
{
    if (list->needs_sorting) {
        list->needs_sorting = 0;
        sort_render_item(list->items, list->n_items);
    }

    v2_i32 camoffset = {0};

    for (i32 n = 0; n < list->n_items; n++) {
        render_item_s *item = &list->items[n];
        item->f(ctx, g, camoffset, item->arg);
    }
}