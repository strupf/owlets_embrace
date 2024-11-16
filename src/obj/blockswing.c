// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32    len;
    v2_i32 anchor;
    //
    v2_i32 p_q8;
    v2_i32 v_q8;
} blockswing_s;

void blockswing_load(g_s *g, map_obj_s *mo)
{
    obj_s        *o  = obj_create(g);
    blockswing_s *bs = (blockswing_s *)o->mem;
    o->ID            = OBJ_ID_BLOCKSWING;
    o->pos.x         = mo->x;
    o->pos.y         = mo->y;
    o->w             = mo->w;
    o->h             = mo->h;
    o->mass          = 2;
    o->flags         = OBJ_FLAG_RENDER_AABB;
}

void blockswing_on_update(g_s *g, obj_s *o)
{
    obj_s        *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    blockswing_s *bs    = (blockswing_s *)o->mem;

    bs->v_q8.y += 100;
    bs->p_q8 = v2_add(bs->p_q8, bs->v_q8);

    v2_i32 anchor_q8 = v2_shl(bs->anchor, 8);

    v2_i32 dt = v2_sub(bs->p_q8, anchor_q8);
    dt        = v2_setlen(dt, bs->len << 8);
}

void blockswing_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    blockswing_s *bs  = (blockswing_s *)o->mem;
    gfx_ctx_s     ctx = gfx_ctx_display();

    v2_i32 ptop = {o->pos.x + o->w / 2, o->pos.y};
    v2_i32 p1   = v2_add(ptop, cam);
    v2_i32 p2   = v2_add(bs->anchor, cam);

    gfx_lin_thick(ctx, p1, p2, GFX_COL_BLACK, 6);
}