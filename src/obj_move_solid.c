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

#if 0
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
                wirenode_move(g, &gh->wire, gh->rn, sx, sy);
                grapplinghook_rope_intact(g, gh);
            }
        }

        if (gh->state) {
            wire_moved_by_aabb(g, &gh->wire, r1, sx, sy);
        }
    }
#endif

#if 1
    for (i32 n = 0; n < g->n_ropes; n++) {
        wire_s *r = g->ropes[n];
        wire_moved_by_aabb(g, r, r1, sx, sy);
    }
#else
    for (i32 n = 0; n < g->n_ropes; n++) {
        rope_handle_s *rh = &g->ropes[n];
        wire_s        *r  = rh->r;

        if (rh->rn_head && overlap_rec_pnt(r2, rh->rn_head->p)) {
            wirenode_s *rn      = rh->rn_head;
            b32         blocked = map_blocked_pt(g, rn->p.x + sx, rn->p.y + sy);

            if (blocked) {
                rh->on_blocked(rh->ctx, r);
            } else {
                wirenode_move(g, r, rn, sx, sy);
            }
        }
        if (rh->rn_tail && overlap_rec_pnt(r2, rh->rn_tail->p)) {
            wirenode_s *rn      = rh->rn_tail;
            b32         blocked = map_blocked_pt(g, rn->p.x + sx, rn->p.y + sy);

            if (blocked) {
                rh->on_blocked(rh->ctx, r);
            } else {
                wirenode_move(g, r, rn, sx, sy);
            }
        }

        wire_moved_by_aabb(g, r, r1, sx, sy);
    }
#endif

    o->pos.x += sx;
    o->pos.y += sy;

#if 0
    if (gh && gh->state && !gh_was_pushed && o == obj_from_obj_handle(gh->o2)) {
        if (map_blocked_pt(g, gh->p.x, gh->p.y) ||
            map_blocked_pt(g, gh->p.x + sx, gh->p.y + sy)) {
            grapplinghook_destroy(g, gh);
        } else {
            gh->p.x += sx;
            gh->p.y += sy;
            // pltf_log("m2 %i | %i\n", sy, g->save.tick);
            wirenode_move(g, &gh->wire, gh->rn, sx, sy);
            grapplinghook_rope_intact(g, gh);
        }
    }
#endif

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
            if (pushed && i->on_pushed_by_solid) {
                i->on_pushed_by_solid(g, i, o, sx, sy);
            }
        }
    }
    o->flags |= OBJ_FLAG_SOLID;
}