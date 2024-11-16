// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void webbounce_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->w     = mo->w;
    o->h     = 16;
    o->pos.x = mo->x;
    o->pos.y = mo->y;
}

void webbounce_on_animate(g_s *g, obj_s *o)
{
}

void webbounce_on_bounced(g_s *g, obj_s *o)
{
}