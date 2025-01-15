// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *boulder_spawn(g_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJID_BOULDER;
    o->w     = 32;
    o->h     = 32;

    return o;
}

void boulder_on_update(g_s *g, obj_s *o)
{
}

void boulder_on_animate(g_s *g, obj_s *o)
{
}