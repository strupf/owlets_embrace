// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 anchor;
} blockswing_s;

void blockswing_on_update(g_s *g, obj_s *o);
void blockswing_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void blockswing_create(g_s *g, map_obj_s *mo)
{
    obj_s *o     = obj_create(g);
    o->on_update = blockswing_on_update;
    o->on_draw   = blockswing_on_draw;
    o->pos.x     = mo->x;
    o->pos.y     = mo->y;
    o->w         = mo->w;
    o->h         = mo->h;
    o->flags     = OBJ_FLAG_SOLID | OBJ_FLAG_RENDER_AABB;
}

void blockswing_on_update(g_s *g, obj_s *o)
{
}