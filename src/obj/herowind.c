// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void herowind_on_animate(g_s *g, obj_s *o);
void herowind_on_update(g_s *g, obj_s *o);
void herowind_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void herowind_load(g_s *g, map_obj_s *mo)
{
    obj_s *o      = obj_create(g);
    o->ID         = 1;
    o->on_update  = herowind_on_update;
    o->on_draw    = herowind_on_draw;
    o->on_animate = herowind_on_animate;
}

void herowind_on_animate(g_s *g, obj_s *o)
{
}

void herowind_on_update(g_s *g, obj_s *o)
{
}

void herowind_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
}
