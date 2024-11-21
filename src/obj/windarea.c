// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void windarea_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_WINDAREA;
    o->pos.x = mo->x;
    o->pos.y = mo->y;
    o->w     = mo->w;
    o->h     = mo->h;
}

void windarea_on_animate(g_s *g, obj_s *o)
{
}

void windarea_on_update(g_s *g, obj_s *o)
{
    obj_s *oh = obj_get_hero(g);
    if (!oh) return;

    if (!overlap_rec(obj_aabb(oh), obj_aabb(o))) return;

    oh->v_q8.y -= 90;
}

void windarea_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
}
