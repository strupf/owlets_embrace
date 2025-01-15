// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 pog;
} dummysolid_s;

void dummysolid_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJID_DUMMYSOLID;
    o->flags = // OBJ_FLAG_SOLID |
        OBJ_FLAG_RENDER_AABB;
    o->flags |= OBJ_FLAG_HOOKABLE;
    dummysolid_s *d = (dummysolid_s *)o->mem;
    o->pos.x        = mo->x;
    o->pos.y        = mo->y;
    o->w            = mo->w;
    o->h            = mo->h;
    d->pog.x        = o->pos.x;
    d->pog.y        = o->pos.y - 50;
}

void dummysolid_on_update(g_s *g, obj_s *o)
{
    dummysolid_s *d = (dummysolid_s *)o->mem;
    o->timer++;
    i32    t  = o->timer << 10;
    v2_i32 pt = {
        (50 * sin_q15(t)) >> 15,
        (50 * cos_q15(t)) >> 15};
    v2_i32 p  = v2_add(pt, d->pog);
    v2_i32 dt = v2_sub(p, o->pos);
    obj_move(g, o, dt.x, dt.y);
}