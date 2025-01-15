// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void light_load(g_s *g, map_obj_s *mo)
{
    obj_s *o          = obj_create(g);
    o->ID             = OBJID_LIGHT;
    o->flags          = OBJ_FLAG_LIGHT;
    o->pos.x          = mo->x;
    o->pos.y          = mo->y;
    o->light_radius   = max_i32(100, map_obj_i32(mo, "R"));
    o->light_strength = map_obj_i32(mo, "Strength");
}

void light_update(g_s *g, obj_s *o)
{
    o->animation++;
}