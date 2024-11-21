// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

void fallingblock_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);

    o->ID         = OBJ_ID_FALLINGBLOCK;
    o->mass       = 1;
    o->moverflags = OBJ_MOVER_MAP;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->w          = mo->w;
    o->h          = mo->h;
}

void fallingblock_on_update(g_s *g, obj_s *o)
{
    if (g->pixel_y + 8 <= o->pos.y) {
        o->pos.y = -o->h - 8;
    }

    o->timer++;
    if (1 < o->timer) {
        o->timer = 0;
        obj_move(g, o, 0, 2);
    }
}

void fallingblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    render_tile_terrain_block(ctx, v2_add(o->pos, cam),
                              o->w >> 4, o->h >> 4,
                              TILE_TYPE_STONE_ROUND_DARK);
}