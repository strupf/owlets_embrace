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

    grapplinghook_s *gh            = &g->ghook;
    bool32           gh_was_pushed = 0;

    if (gh->state) {
        grapplinghook_try_grab_obj(g, gh, o, -sx, -sy);

        if (overlap_rec_pnt(r2, gh->p)) {
            o->flags &= ~OBJ_FLAG_SOLID;
            b32 gh_blocked = map_blocked_pt(g, gh->p.x + sx, gh->p.y + sy);
            o->flags |= OBJ_FLAG_SOLID;
            if (gh_blocked) {
                grapplinghook_destroy(g, gh);
            } else {
                gh_was_pushed = 1;
                gh->p.x += sx;
                gh->p.y += sy;
                // pltf_log("m1 %i | %i\n", sy, g->save.tick);
                ropenode_move(g, &gh->rope, gh->rn, sx, sy);
                grapplinghook_rope_intact(g, gh);
            }
        }

        if (gh->state) {
            rope_moved_by_aabb(g, &gh->rope, r1, sx, sy);
        }
    }
    o->pos.x += sx;
    o->pos.y += sy;

    if (gh && gh->state && !gh_was_pushed && o == obj_from_obj_handle(gh->o2)) {
        if (map_blocked_pt(g, gh->p.x, gh->p.y) ||
            map_blocked_pt(g, gh->p.x + sx, gh->p.y + sy)) {
            grapplinghook_destroy(g, gh);
        } else {
            gh->p.x += sx;
            gh->p.y += sy;
            // pltf_log("m2 %i | %i\n", sy, g->save.tick);
            ropenode_move(g, &gh->rope, gh->rn, sx, sy);
            grapplinghook_rope_intact(g, gh);
        }
    }

    o->flags &= ~OBJ_FLAG_SOLID;
    for (obj_each(g, i)) {
        if ((i->flags & OBJ_FLAG_ACTOR) &&
            (i->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS)) {
            b32 linked = o == obj_from_obj_handle(i->linked_solid);
            b32 pushed = overlap_rec(r2, obj_aabb(i));
            b32 riding = overlap_rec(r1, obj_rec_bottom(i));

            if (linked | pushed | riding) {
                obj_step_actor(g, i, sx, +0);
                obj_step_actor(g, i, +0, sy);
            }
        }
    }
    o->flags |= OBJ_FLAG_SOLID;
}