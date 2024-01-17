// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *carrier_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_CARRIER;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_PLATFORM |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_RENDER_AABB;
    o->w            = 32;
    o->h            = 32;
    o->drag_q8.y    = 255;
    o->vel_cap_q8.x = 3000;
    o->gravity_q8.y = 40;
    return o;
}

void carrier_on_update(game_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = 0;
    }
    if (o->bumpflags & OBJ_BUMPED_X) {
        o->vel_q8.x = 0;
    }
    o->bumpflags = 0;
    o->drag_q8.x = 250;

    obj_s *hero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!hero) return;

    rec_i32 rtop      = {o->pos.x, o->pos.y, o->w, 1};
    rec_i32 rherofeet = obj_rec_bottom(hero);
    if (!overlap_rec(rherofeet, rtop)) return;

    if (obj_grounded(g, o)) {
        o->vel_q8.x += hero->facing * 50;
        o->drag_q8.x = 256;
    }
}
