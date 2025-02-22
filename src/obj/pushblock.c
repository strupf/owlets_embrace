// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

i32  pushblock_on_pushpull(g_s *g, obj_s *o, i32 dir);
void pushblock_on_update(g_s *g, obj_s *o);
void pushblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void pushblock_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJID_PUSHBLOCK;

    i32 w              = map_obj_i32(mo, "Weight");
    o->substate        = 1 < w ? w : 1;
    o->on_update       = pushblock_on_update;
    o->on_draw         = pushblock_on_draw;
    o->on_pushpull     = pushblock_on_pushpull;
    o->flags           = OBJ_FLAG_SOLID | OBJ_FLAG_GRAB;
    o->render_priority = RENDER_PRIO_HERO + 1;

    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->w          = mo->w;
    o->h          = mo->h;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS;
}

void pushblock_on_update(g_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->timer  = 0; // reset fall delay timer
        o->v_q8.y = 0;
    }
    o->bumpflags = 0;
    if (map_blocked(g, obj_rec_bottom(o))) {
        o->timer  = 0; // reset fall delay timer
        o->v_q8.y = 0;
    } else {
        // not grounded -> don't push, only fall
        o->timer++; // fall delay timer
        o->v_q8.y += (15 <= o->timer ? 60 : 0);
        o->v_q8.y = min_i32(o->v_q8.y, 256 * 10);
        obj_move_by_v_q8(g, o);
    }
}

void pushblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    v2_i32 pos = v2_i32_add(o->pos, cam);
    render_tile_terrain_block(gfx_ctx_display(), pos, o->w / 16, o->h / 16, TILE_TYPE_BRIGHT_STONE);
}

i32 pushblock_on_pushpull(g_s *g, obj_s *o, i32 dir)
{
    if (!map_blocked(g, obj_rec_bottom(o))) {
        return 0;
    } else {
        // push speed
        if (gameplay_time(g) % o->substate) return 0;

        obj_move(g, o, dir, 0);
        return 1;
    }
}