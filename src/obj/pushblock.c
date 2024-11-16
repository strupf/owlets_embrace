// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

void pushblock_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_PUSHBLOCK;

    i32 w         = map_obj_i32(mo, "Weight");
    o->substate   = 1 < w ? w : 1;
    o->mass       = 2;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->w          = mo->w;
    o->h          = mo->h;
    o->moverflags = OBJ_MOVER_MAP;
}

void pushblock_on_update(g_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->timer  = 0; // reset fall delay timer
        o->v_q8.y = 0;
    }
    o->bumpflags = 0;

    if (!map_blocked(g, o, obj_rec_bottom(o), o->mass)) {
        // not grounded -> don't push, only fall
        o->timer++; // fall delay timer
        o->v_q8.y += (5 <= o->timer ? 80 : 0);
        obj_move_by_v_q8(g, o);
    } else {
        // push speed
        if (gameplay_time(g) % o->substate) return;

        // check if being pushed
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!ohero) return;
        if (!obj_grounded(g, ohero)) return;

        hero_s *h        = (hero_s *)ohero->heap;
        rec_i32 heroaabb = obj_aabb(ohero);
        rec_i32 rl       = obj_rec_left(o);
        rec_i32 rr       = obj_rec_right(o);
        i32     dpad_x   = inp_x();

        if (dpad_x == +1 && overlap_rec(heroaabb, rl)) {
            h->pushing = dpad_x;
            if (obj_step(g, o, +1, +0, 0, 0)) {
                obj_step(g, ohero, +1, +0, 1, 0);
            }
        }
        if (dpad_x == -1 && overlap_rec(heroaabb, rr)) {
            h->pushing = dpad_x;
            if (obj_step(g, o, -1, +0, 0, 0)) {
                obj_step(g, ohero, -1, +0, 1, 0);
            }
        }
    }
}

void pushblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    v2_i32 pos = v2_add(o->pos, cam);
    render_tile_terrain_block(gfx_ctx_display(), pos, o->w / 16, o->h / 16, TILE_TYPE_STONE_SQUARE_DARK);
}