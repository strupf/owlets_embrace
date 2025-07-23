// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void bigcrab_on_animate(g_s *g, obj_s *o);
void bigcrab_on_update(g_s *g, obj_s *o);
void bigcrab_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void bigcrab_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJID_BIGCRAB;
    o->w     = 64;
    o->h     = 48;
    obj_place_to_map_obj(o, mo, 0, 1);
    o->on_animate = bigcrab_on_animate;
    o->on_update  = bigcrab_on_update;
    o->on_draw    = bigcrab_on_draw;
}

void bigcrab_on_animate(g_s *g, obj_s *o)
{
}

void bigcrab_on_update(g_s *g, obj_s *o)
{
}

void bigcrab_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
}