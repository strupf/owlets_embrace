// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// falling block which respawns back at the top after falling across the screen

#include "game.h"

void fallingblock_on_update(g_s *g, obj_s *o);
void fallingblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void fallingblock_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);

    o->ID              = OBJID_FALLINGBLOCK;
    o->flags           = OBJ_FLAG_SOLID | OBJ_FLAG_CLIMBABLE;
    // o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;
    o->render_priority = RENDER_PRIO_OWL + 1;
    o->on_update       = fallingblock_on_update;
    o->on_draw         = fallingblock_on_draw;
}

void fallingblock_on_update(g_s *g, obj_s *o)
{
    if (g->pixel_y + 32 <= o->pos.y) {
        o->pos.y -= g->pixel_y + 128;
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
    v2_i32    pos = v2_i32_add(o->pos, cam);
    pos.x &= ~1;
    pos.y &= ~1;
    render_tile_terrain_block(ctx, pos,
                              o->w >> 4, o->h >> 4,
                              TILE_TYPE_BRIGHT_STONE);
}