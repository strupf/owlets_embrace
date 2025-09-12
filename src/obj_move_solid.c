// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void obj_step_solid(g_s *g, obj_s *o, i32 sx, i32 sy);

void obj_move_solid(g_s *g, obj_s *o, i32 dx, i32 dy)
{
    for (i32 m = abs_i32(dx), s = 0 < dx ? +1 : -1; m; m--) {
        obj_step_solid(g, o, s, 0);
    }
    for (i32 m = abs_i32(dy), s = 0 < dy ? +1 : -1; m; m--) {
        obj_step_solid(g, o, 0, s);
    }
}

void obj_step_solid(g_s *g, obj_s *o, i32 sx, i32 sy)
{
    assert(!(o->flags & OBJ_FLAG_ACTOR) && (o->flags & OBJ_FLAG_SOLID));
    assert((abs_i32(sx) == 1 && sy == 0) || (abs_i32(sy) == 1 && sx == 0));

    rec_i32 r1 = {o->pos.x, o->pos.y, o->w, o->h};
    rec_i32 r2 = {o->pos.x + sx, o->pos.y + sy, o->w, o->h};

    for (i32 n = 0; n < g->n_ropes; n++) {
        wire_s *r = g->ropes[n];
        wire_moved_by_aabb(g, r, r1, sx, sy);
    }

    o->pos.x += sx;
    o->pos.y += sy;
    o->flags &= ~OBJ_FLAG_SOLID;

    for (obj_each(g, i)) {
        if ((i->flags & OBJ_FLAG_ACTOR) &&
            (i->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS)) {
            b32 linked = o == obj_from_handle(i->linked_solid);
            b32 pushed = overlap_rec(r2, obj_aabb(i));
            b32 riding = overlap_rec(r1, obj_rec_bottom(i));

            if (linked | pushed | riding) {
                obj_step_actor(g, i, sx, +0);
                obj_step_actor(g, i, +0, sy);
            }
            if (pushed && i->on_pushed_by_solid) {
                i->on_pushed_by_solid(g, i, o, sx, sy);
            }
        }
    }
    o->flags |= OBJ_FLAG_SOLID;
}