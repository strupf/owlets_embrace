// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32 x;
} pushablebox_s;

void pushablebox_on_update(game_s *g, obj_s *o)
{
    pushablebox_s *box = (pushablebox_s *)o->mem;

    switch (o->state) {
    case 0: { // static
        if (sys_tick() & 1) break;
        int dpx = inp_dpad_x();

        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!ohero || !dpx) {
            break;
        }

        rec_i32 pushrh = dpx < 0 ? obj_rec_left(ohero) : obj_rec_right(ohero);
        rec_i32 pushrb = dpx < 0 ? obj_rec_left(o) : obj_rec_right(o);
        if (!overlap_rec(obj_aabb(o), pushrh)) {
            break;
        }

        o->flags &= ~OBJ_FLAG_SOLID;
        bool32 herocouldmove = game_traversable(g, pushrh);
        bool32 boxcouldmove  = game_traversable(g, pushrb);
        o->flags |= OBJ_FLAG_SOLID;
        if (!herocouldmove || !boxcouldmove) break;
        obj_move(g, o, (v2_i32){dpx, 0});

        break;
    }
    case 1: { // moving
        break;
    }
    }
}

void pushablebox_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_PUSHABLEBOX;
    o->flags = OBJ_FLAG_SOLID |
               OBJ_FLAG_RENDER_AABB;
    o->on_update       = pushablebox_on_update;
    pushablebox_s *box = (pushablebox_s *)o->mem;
    o->moverflags      = OBJ_MOVER_GLUE_GROUND;
    o->gravity_q8.y    = 70;
    o->drag_q8.x       = 250;
    o->drag_q8.y       = 256;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;
}
