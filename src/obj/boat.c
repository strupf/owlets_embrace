// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void boat_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_BOAT;
    o->flags = OBJ_FLAG_SOLID |
               OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_SPRITE;
    o->pos.x = mo->x;
    o->pos.y = mo->y;
    o->w     = 64;
    o->h     = 32;
}

void boat_on_update(game_s *g, obj_s *o)
{
    int x1 = o->pos.x;
    int x2 = o->pos.x + o->w - 1;
    int h1 = ocean_height(g, x1);
    int h2 = ocean_height(g, x2);
    int ht = (h1 + h2) >> 1;
    int dy = ht - (o->pos.y + 16);

    o->tomove.y = dy;

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);

    rec_i32 rr = obj_rec_right(o);
    rec_i32 rl = obj_rec_left(o);
}

void boat_on_animate(game_s *g, obj_s *o)
{
}