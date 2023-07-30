/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#include "obj.h"
#include "game.h"

bool32 objhandle_is_valid(objhandle_s h)
{
        return (h.o && h.o->gen == h.gen);
}

bool32 objhandle_is_null(objhandle_s h)
{
        return (h.o == NULL);
}

obj_s *obj_from_handle(objhandle_s h)
{
        return (h.o && h.o->gen == h.gen ? h.o : NULL);
}

bool32 try_obj_from_handle(objhandle_s h, obj_s **o)
{
        obj_s *obj = obj_from_handle(h);
        if (o) {
                *o = obj;
                return 1;
        }
        return 0;
}

objhandle_s objhandle_from_obj(obj_s *o)
{
        objhandle_s h = {o->gen, o};
        return h;
}

bool32 obj_contained_in_array(obj_s *o, obj_s **arr, int num)
{
        for (int n = 0; n < num; n++) {
                if (arr[n] == o) return 1;
        }
        return 0;
}

obj_s *obj_create(game_s *g)
{
        ASSERT(g->n_objfree > 0);
        obj_s *o = g->objfreestack[--g->n_objfree];
        ASSERT(!objset_contains(&g->obj_active, o));

        int                index   = o->index;
        int                gen     = o->gen;
        static const obj_s objzero = {0};
        *o                         = objzero;
        o->gen                     = gen;
        o->index                   = index;

        objset_add(&g->obj_active, o);
        return o;
}

void obj_delete(game_s *g, obj_s *o)
{
        ASSERT(!objset_contains(&g->obj_scheduled_delete, o));
        ASSERT(!obj_contained_in_array(o, g->objfreestack, g->n_objfree));
        objset_add(&g->obj_scheduled_delete, o);
}

bool32 obj_is_direct_child(obj_s *o, obj_s *parent)
{
        for (obj_s *it = parent->fchild; it; it = it->next) {
                if (it == o) return 1;
        }
        return 0;
}

bool32 obj_is_child_any_depth(obj_s *o, obj_s *parent)
{
        if (!parent->fchild) return 0;
        obj_s *stack[64] = {parent->fchild};
        int    n         = 1;
        do {
                for (obj_s *it = stack[--n]; it; it = it->next) {
                        if (it == o) return 1;
                        if (it->fchild) {
                                ASSERT(n < ARRLEN(stack));
                                stack[n++] = it->fchild;
                        }
                }
        } while (n > 0);
        return 0;
}

bool32 obj_are_siblings(obj_s *a, obj_s *b)
{
        for (obj_s *o = a->next; o; o = o->next) {
                if (o == b) return 1;
        }
        for (obj_s *o = a->prev; o; o = o->prev) {
                if (o == b) return 1;
        }
        return 0;
}

rec_i32 obj_aabb(obj_s *o)
{
        rec_i32 r = {o->pos.x, o->pos.y, o->w, o->h};
        return r;
}

/* ________
 * | |    |
 * | |AABB|
 * |_|____|
 */
rec_i32 obj_rec_left(obj_s *o)
{
        rec_i32 r = {o->pos.x - 1, o->pos.y, 1, o->h};
        return r;
}

/*   ________
 *   |    | |
 *   |AABB| |
 *   |____|_|
 */
rec_i32 obj_rec_right(obj_s *o)
{
        rec_i32 r = {o->pos.x + o->w, o->pos.y, 1, o->h};
        return r;
}

/*   ______
 *   |    |
 *   |AABB|
 *   |____|
 *   |____|
 */
rec_i32 obj_rec_bottom(obj_s *o)
{
        rec_i32 r = {o->pos.x, o->pos.y + o->h, o->w, 1};
        return r;
}

/*
 *   ______
 *   |____|
 *   |    |
 *   |AABB|
 *   |____|
 */
rec_i32 obj_rec_top(obj_s *o)
{
        rec_i32 r = {o->pos.x, o->pos.y - 1, o->w, 1};
        return r;
}

void obj_move_x(game_s *g, obj_s *o, int dx)
{
        int sx = SGN(dx);
        for (int m = ABS(dx); m > 0; m--) {
                if (!actor_step_x(g, o, sx)) {
                        o->vel_q8.x = 0;
                        break;
                }
        }
}

void obj_move_y(game_s *g, obj_s *o, int dy)
{
        int sy = SGN(dy);
        for (int m = ABS(dy); m > 0; m--) {
                if (!actor_step_y(g, o, sy)) {
                        o->vel_q8.y = 0;
                        break;
                }
        }
}

bool32 actor_step_x(game_s *g, obj_s *o, int sx)
{
        ASSERT(ABS(sx) <= 1);

        rec_i32 r = translate_rec_xy(obj_aabb(o), sx, 0);
        if (!game_area_blocked(g, r)) {
                o->pos.x += sx;
                if (o->actorflags & ACTOR_FLAG_GLUE_GROUND) {
                        rec_i32 r1 = translate_rec_xy(obj_aabb(o), 0, +1);
                        rec_i32 r2 = translate_rec_xy(obj_aabb(o), 0, +2);
                        if (game_area_blocked(g, r1) == 0 &&
                            game_area_blocked(g, r2)) {
                                o->pos.y += 1;
                        }
                }
                return 1;
        }

        // the position to the side is blocked
        // check if object can climb slopes and try it out
        if (o->actorflags & ACTOR_FLAG_CLIMB_SLOPES) {
                rec_i32 r1 = translate_rec_xy(obj_aabb(o), sx, -1);
                if (!game_area_blocked(g, r1)) {
                        o->pos.x += sx;
                        o->pos.y -= 1;
                        return 1;
                }

                if (o->actorflags & ACTOR_FLAG_CLIMB_STEEP_SLOPES) {
                        rec_i32 r2 = translate_rec_xy(obj_aabb(o), sx, -2);
                        if (!game_area_blocked(g, r2)) {
                                o->pos.x += sx;
                                o->pos.y -= 2;
                                return 1;
                        }
                }
        }

        o->vel_q8.x = 0;
        return 0;
}

bool32 actor_step_y(game_s *g, obj_s *o, int sy)
{
        ASSERT(ABS(sy) <= 1);

        rec_i32 r = translate_rec_xy(obj_aabb(o), 0, sy);
        if (!game_area_blocked(g, r)) {
                o->pos.y += sy;
                return 1;
        }

        o->vel_q8.y = 0;
        return 0;
}

void solid_step_x(game_s *g, obj_s *o, int sx)
{
}

void solid_step_y(game_s *g, obj_s *o, int sy)
{
}