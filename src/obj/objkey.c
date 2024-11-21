// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void objkey_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_KEY;
    o->flags = OBJ_FLAG_RENDER_AABB;
    o->pos.x = mo->x;
    o->pos.y = mo->y;
    o->w     = mo->w;
    o->h     = mo->h;
}

void objkey_on_update(g_s *g, obj_s *o)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    o->timer++;

    if (!ohero) return;
    if (!overlap_rec(obj_aabb(o), obj_aabb(ohero))) return;

    hero_inv_add(g, o->substate, 1);
    obj_delete(g, o);
}

void objkey_on_animate(g_s *g, obj_s *o)
{
}