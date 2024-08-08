// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_pickup_on_update(game_s *g, obj_s *o);
void hero_pickup_on_animate(game_s *g, obj_s *o);

obj_s *hero_pickup_create(game_s *g, v2_i32 pos, i32 pickupID)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HERO_PICKUP;
    o->subID = pickupID;
    o->w     = 16;
    o->h     = 16;
    o->flags = OBJ_FLAG_MOVER |
               OBJ_FLAG_RENDER_AABB;
    o->grav_q8.y = 80;
    return o;
}

void hero_pickup_load(game_s *g, map_obj_s *mo)
{
    v2_i32 spawnpos = {0};
    obj_s *o        = hero_pickup_create(g, spawnpos, 0);
}

void hero_pickup_on_update(game_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMPED_X) {
        o->v_q8.x = 0;
    }
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->v_q8.y = 0;
    }
    o->bumpflags = 0;
}

void hero_pickup_on_animate(game_s *g, obj_s *o)
{
}