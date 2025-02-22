// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void pickup_on_update(g_s *g, obj_s *o);
void pickup_on_animate(g_s *g, obj_s *o);

obj_s *pickup_create(g_s *g, v2_i32 p, i32 pickupID)
{
    obj_s *o      = obj_create(g);
    o->on_update  = pickup_on_update;
    o->on_animate = pickup_on_animate;
    o->ID         = OBJID_PICKUP;

    o->subID = pickupID;
    o->w     = 8;
    o->h     = 8;
    o->pos.x = p.x - o->w / 2;
    o->pos.y = p.y - o->h / 2;
    return o;
}

void pickup_load(g_s *g, map_obj_s *mo)
{
    v2_i32 p        = {mo->x + mo->w / 2, mo->y + mo->h / 2};
    i32    pickupID = map_obj_i32(mo, "PickupID");
    obj_s *o        = pickup_create(g, p, pickupID);
}

void pickup_on_update(g_s *g, obj_s *o)
{
}

void pickup_on_animate(g_s *g, obj_s *o)
{
}