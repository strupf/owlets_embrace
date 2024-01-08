// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *carrier_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_CARRIER;
    o->flags = OBJ_FLAG_MOVER |
               OBJ_FLAG_PLATFORM |
               OBJ_FLAG_KILL_OFFSCREEN;
    o->w            = 32;
    o->h            = 32;
    o->drag_q8.x    = 230;
    o->drag_q8.y    = 255;
    o->gravity_q8.y = 30;
    return o;
}

void carrier_on_update(game_s *g, obj_s *o)
{
    obj_s *hero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!hero) return;

    rec_i32 r = {o->pos.x, o->pos.y, o->w, 1};
    if (!overlap_rec(obj_rec_bottom(hero), r)) return;

    int vtarget = hero->facing * 1000;
    if (obj_grounded(g, o)) {
        o->vel_q8.x += sgn_i(vtarget) * 5;
    }
    o->vel_q8.x = clamp_i(o->vel_q8.x, -1000, +1000);
}
