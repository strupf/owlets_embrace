// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

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
        if (obj) {
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

void obj_set_flags(game_s *g, obj_s *o, objflags_s flags)
{
        o->flags = flags;
        for (int n = 0; n < NUM_OBJ_BUCKETS; n++) {
                objbucket_s *b = &g->objbuckets[n];
                objflags_s   f = flags;

                f = objflags_op(f, b->op_flag[0], b->op_func[0]);
                f = objflags_op(f, b->op_flag[1], b->op_func[1]);

                if (objflags_cmp(f, b->cmp_flag, b->cmp_func)) {
                        objset_add(&b->set, o);
                } else {
                        objset_del(&b->set, o);
                }
        }
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

v2_i32 obj_aabb_center(obj_s *o)
{
        v2_i32 p = {o->pos.x + (o->w / 2),
                    o->pos.y + (o->h / 2)};
        return p;
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

/*   ______
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

static bool32 actor_try_wiggle(game_s *g, obj_s *o)
{
        rec_i32 aabb = obj_aabb(o);
        if (!game_area_blocked(g, aabb)) return 1;

        for (int y = -1; y <= +1; y++) {
                for (int x = -1; x <= +1; x++) {
                        rec_i32 r = translate_rec_xy(aabb, x, y);
                        if (!game_area_blocked(g, r)) {
                                o->pos.x += x;
                                o->pos.y += y;
                                return 1;
                        }
                }
        }
        o->squeezed = 1;
        return 0;
}

static inline i_actor_step(game_s *g, obj_s *o, int sx, int sy)
{
        o->pos.x += sx;
        o->pos.y += sy;
        if (o->rope && o->ropenode) {
                v2_i32 d = {sx, sy};
                ropenode_move(g, o->rope, o->ropenode, d);
        }
}

bool32 actor_step_x(game_s *g, obj_s *o, int sx)
{
        ASSERT(ABS(sx) <= 1);

        rec_i32 r = translate_rec_xy(obj_aabb(o), sx, 0);
        if (!game_area_blocked(g, r)) {
                i_actor_step(g, o, sx, 0);

                if (o->actorflags & ACTOR_FLAG_GLUE_GROUND) {
                        rec_i32 r1 = translate_rec_xy(obj_aabb(o), 0, +1);
                        rec_i32 r2 = translate_rec_xy(obj_aabb(o), 0, +2);
                        if (game_area_blocked(g, r1) == 0 &&
                            game_area_blocked(g, r2)) {
                                i_actor_step(g, o, 0, 1);
                        }
                }
                return 1;
        }

        // the position to the side is blocked
        // check if object can climb slopes and try it out
        if (o->actorflags & ACTOR_FLAG_CLIMB_SLOPES) {
                rec_i32 r1 = translate_rec_xy(obj_aabb(o), sx, -1);
                if (!game_area_blocked(g, r1)) {
                        i_actor_step(g, o, sx, -1);
                        return 1;
                }

                if (o->actorflags & ACTOR_FLAG_CLIMB_STEEP_SLOPES) {
                        rec_i32 r2 = translate_rec_xy(obj_aabb(o), sx, -2);
                        if (!game_area_blocked(g, r2)) {
                                i_actor_step(g, o, sx, -2);
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
                i_actor_step(g, o, 0, sy);
                return 1;
        }

        o->vel_q8.y = 0;
        return 0;
}

static void solid_step(game_s *g, obj_s *o, int sx, int sy)
{
        ASSERT((ABS(sx) == 1 && sy == 0) || (ABS(sy) == 1 && sx == 0));

        obj_s *hero;
        if (try_obj_from_handle(g->hero.obj, &hero) && hero->rope) {
                o->soliddisabled = 1;
                rope_moved_by_solid(g, hero->rope, o, (v2_i32){sx, sy});
                o->soliddisabled = 0;
        }

        o->pos.x += sx;
        o->pos.y += sy;
        rec_i32     r      = obj_aabb(o);
        obj_listc_s actors = objbucket_list(g, OBJ_BUCKET_ACTOR);
        for (int n = 0; n < actors.n; n++) {
                obj_s *a = actors.o[n];
                if (a->squeezed) continue;
                rec_i32 aabb  = obj_aabb(a);
                rec_i32 rfeet = translate_rec_xy(obj_rec_bottom(a), sx, sy);
                if (overlap_rec_excl(r, aabb) || overlap_rec_excl(r, rfeet)) {
                        if (sx != 0) actor_step_x(g, a, sx);
                        if (sy != 0) actor_step_y(g, a, sy);
                        if (!actor_try_wiggle(g, a) && a->onsqueeze) {
                                a->onsqueeze(g, a);
                        }
                }
        }
}

void solid_move(game_s *g, obj_s *o, int dx, int dy)
{
        int sx = SGN(dx);
        for (int m = ABS(dx); m > 0; m--) {
                solid_step(g, o, sx, 0);
        }
        int sy = SGN(dy);
        for (int m = ABS(dy); m > 0; m--) {
                solid_step(g, o, 0, sy);
        }
}

// apply gravity, drag, modify subposition and write pos_new
// uses subpixel position:
// subposition is [0, 255]. If the boundaries are exceeded
// the goal is to move a whole pixel left or right
void obj_apply_movement(obj_s *o)
{
        o->vel_q8    = v2_add(o->vel_q8, o->gravity_q8);
        o->vel_q8.x  = q_mulr(o->vel_q8.x, o->drag_q8.x, 8);
        o->vel_q8.y  = q_mulr(o->vel_q8.y, o->drag_q8.y, 8);
        o->subpos_q8 = v2_add(o->subpos_q8, o->vel_q8);
        o->pos_new   = v2_add(o->pos, v2_shr(o->subpos_q8, 8));
        o->subpos_q8.x &= 255;
        o->subpos_q8.y &= 255;
}

void interact_open_dialogue(game_s *g, obj_s *o)
{
        char filename[64] = "assets/";
        os_strcat(filename, o->dialogue);
        textbox_load_dialog(&g->textbox, filename);
}