// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "obj.h"
#include "game.h"
#include "rope.h"

obj_s *obj_get_tagged(game_s *g, int tag)
{
        obj_s *o = g->obj_tag[tag];
        return o;
}

bool32 obj_tag(game_s *g, obj_s *o, int tag)
{
        flags32 t = 1U << tag;
        if (obj_is_tagged(o, tag)) return 0;
        if (obj_get_tagged(g, tag)) return 0;
        g->obj_tag[tag] = o;
        o->tags |= t;
        return 1;
}

bool32 obj_untag(game_s *g, obj_s *o, int tag)
{
        flags32 t = 1U << tag;
        if (obj_is_tagged(o, tag)) {
                o->tags &= ~t;
                ASSERT(g->obj_tag[tag] == o);
                g->obj_tag[tag] = NULL;
                return 1;
        }
        return 0;
}

bool32 obj_is_tagged(obj_s *o, int tag)
{
        flags32 t = 1U << tag;
        return (o->tags & t);
}

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

objhandle_s objhandle_from_obj(obj_s *o)
{
        objhandle_s h = {o->gen, o};
        return h;
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

obj_s *obj_create(game_s *g)
{
        ASSERT(g->n_objfree > 0);
        obj_s *o = g->objfreestack[--g->n_objfree];
        ASSERT(!objset_contains(&g->objbuckets[0].set, o));

        int index = o->index;
        int gen   = o->gen;

        obj_generic_s *og = (obj_generic_s *)o;
        *og               = (const obj_generic_s){0};
        og->magic         = MAGIC_NUM_OBJ_2;
        o->magic          = MAGIC_NUM_OBJ_1;
        o->gen            = gen;
        o->index          = index;

        objset_add(&g->objbuckets[0].set, o);

        return o;
}

void obj_delete(game_s *g, obj_s *o)
{
        ASSERT(!objset_contains(&g->obj_scheduled_delete, o));
        ASSERT(!obj_contained_in_array(o, g->objfreestack, g->n_objfree));
        if (o->ondelete) { // execute ondelete exactly once
                o->ondelete(g, o);
                o->ondelete = NULL;
        }
        objset_add(&g->obj_scheduled_delete, o);
}

bool32 obj_contained_in_array(const obj_s *o, const obj_s **arr, int num)
{
        return ptr_find(o, (const void **)arr, num) >= 0;
}

v2_i32 obj_aabb_center(obj_s *o)
{
        v2_i32 p = {o->pos.x + (o->w >> 1), o->pos.y + (o->h >> 1)};
        return p;
}

rec_i32 obj_aabb(obj_s *o)
{
        rec_i32 r = {o->pos.x, o->pos.y, o->w, o->h};
        return r;
}

rec_i32 obj_rec_left(obj_s *o)
{
        rec_i32 r = {o->pos.x - 1, o->pos.y, 1, o->h};
        return r;
}

rec_i32 obj_rec_right(obj_s *o)
{
        rec_i32 r = {o->pos.x + o->w, o->pos.y, 1, o->h};
        return r;
}

rec_i32 obj_rec_bottom(obj_s *o)
{
        rec_i32 r = {o->pos.x, o->pos.y + o->h, o->w, 1};
        return r;
}

rec_i32 obj_rec_top(obj_s *o)
{
        rec_i32 r = {o->pos.x, o->pos.y - 1, o->w, 1};
        return r;
}

void obj_apply_flags(game_s *g, obj_s *o, flags64 flags)
{
        o->flags = flags;
        for (int n = 0; n < NUM_OBJ_BUCKETS; n++) {
                objbucket_s *b = &g->objbuckets[n];
                flags64      f = flags;

                f = objflags_op(f, b->op_flag[0], b->op_func[0]);
                f = objflags_op(f, b->op_flag[1], b->op_func[1]);
                if (objflags_cmp(f, b->cmp_flag, b->cmp_func)) {
                        objset_add(&b->set, o);
                } else {
                        objset_del(&b->set, o);
                }
        }
}

void obj_set_flags(game_s *g, obj_s *o, flags64 flags)
{
        obj_apply_flags(g, o, o->flags | flags);
}

void obj_unset_flags(game_s *g, obj_s *o, flags64 flags)
{
        obj_apply_flags(g, o, o->flags & ~flags);
}

void obj_interact_open_dialog(game_s *g, obj_s *o)
{
        textbox_load_dialog(&g->textbox, o->filename);
}

obj_s *obj_closest_interactable(game_s *g, v2_i32 pos)
{
        obj_s      *closest       = NULL;
        u32         closestdist   = INTERACTABLE_DISTSQ;
        obj_listc_s interactables = objbucket_list(g, OBJ_BUCKET_INTERACT);
        for (int n = 0; n < interactables.n; n++) {
                obj_s *o      = interactables.o[n];
                u32    distsq = v2_distancesq(obj_aabb_center(o), pos);
                if (distsq < closestdist) {
                        closestdist = distsq;
                        closest     = o;
                }
        }
        return closest;
}

int obj_get_with_ID(game_s *g, int ID, objset_s *set)
{
        obj_listc_s l = obj_list_all(g);
        int         i = 0;
        for (int n = 0; n < l.n; n++) {
                obj_s *o = l.o[n];
                if (o->ID == ID) {
                        objset_add(set, o);
                        i++;
                }
        }
        return i;
}

bool32 obj_overlaps_spikes(game_s *g, obj_s *o)
{
        return (room_overlaps_tileID(g, obj_aabb(o), TILE_SPIKES));
}

objset_s *objset_create(void *(*allocfunc)(size_t))
{
        objset_s *set = (objset_s *)allocfunc(sizeof(objset_s));
        objset_clr(set);
        return set;
}

bool32 objset_add(objset_s *set, obj_s *o)
{
        int i = o->index;
        if (i == 0 || set->s[i] != 0) return 0;
        int n     = ++set->n;
        set->s[i] = n;
        set->d[n] = i;
        set->o[n] = o;
        return 1;
}

bool32 objset_del(objset_s *set, obj_s *o)
{
        int i = o->index;
        int j = set->s[i];
        if (j == 0) return 0;
        int n     = set->n--;
        int k     = set->d[n];
        set->d[n] = 0;
        set->d[j] = k;
        set->s[k] = j;
        set->s[i] = 0;
        set->o[j] = set->o[n];
        return 1;
}

bool32 objset_contains(objset_s *set, obj_s *o)
{
        int i = o->index;
        return (set->s[i] != 0);
}

void objset_clr(objset_s *set)
{
        if (set->n > 0) {
                *set = (const objset_s){0};
        }
}

int objset_len(objset_s *set)
{
        return set->n;
}

obj_s *objset_at(objset_s *set, int i)
{
        ASSERT(0 <= i && i < set->n);
        return set->o[1 + i];
}

obj_listc_s objset_list(objset_s *set)
{
        obj_listc_s l = {&set->o[1], set->n};
        return l;
}

static void objset_swap(objset_s *set, int i, int j)
{
        int si = set->s[i];
        int sj = set->s[j];
        SWAP(obj_s *, set->o[si], set->o[sj]);
        SWAP(int, set->d[si], set->d[sj]);
        SWAP(int, set->s[i], set->s[j]);
}

void objset_sort(objset_s *set, int (*cmpf)(const obj_s *a, const obj_s *b))
{
        while (1) {
                bool32 swapped = 0;
                for (int n = 1; n < set->n; n++) {
                        obj_s *a = set->o[n + 1];
                        obj_s *b = set->o[n];
                        if (cmpf(a, b) > 0) {
                                objset_swap(set, a->index, b->index);
                                swapped = 1;
                        }
                }
                if (!swapped) break;
        }
}

void objset_print(objset_s *set)
{
        for (int n = 0; n < NUM_OBJS; n++) {
                PRINTF("%i| s: %i d: %i | o: %i %i\n", n, set->s[n], set->d[n], set->o[n] ? set->o[n]->index : -1, set->o[n] ? set->o[n]->ID : -1);
        }
}

void objset_filter_overlap_circ(objset_s *set, v2_i32 p, i32 r, bool32 inv)
{
        obj_s      *to_remove[NUM_OBJS];
        int         n_remove = 0;
        obj_listc_s l        = objset_list(set);
        for (int n = 0; n < l.n; n++) {
                obj_s *o = l.o[n];
                if (overlap_rec_circ(obj_aabb(o), p, r) != inv) {
                        to_remove[n_remove++] = o;
                }
        }

        for (int n = 0; n < n_remove; n++) {
                objset_del(set, to_remove[n]);
        }
}

void objset_filter_in_distance(objset_s *set, v2_i32 p, i32 r, bool32 inv)
{
        obj_s      *to_remove[NUM_OBJS];
        int         n_remove = 0;
        obj_listc_s l        = objset_list(set);
        u32         r2       = (u32)r * (u32)r;
        for (int n = 0; n < l.n; n++) {
                obj_s *o = l.o[n];
                if ((v2_distancesq(obj_aabb_center(o), p) > r2) != inv) {
                        to_remove[n_remove++] = o;
                }
        }

        for (int n = 0; n < n_remove; n++) {
                objset_del(set, to_remove[n]);
        }
}

void objset_filter_overlap_rec(objset_s *set, rec_i32 r, bool32 inv)
{
        obj_s      *to_remove[NUM_OBJS];
        int         n_remove = 0;
        obj_listc_s l        = objset_list(set);
        for (int n = 0; n < l.n; n++) {
                obj_s *o = l.o[n];
                if (overlap_rec_excl(obj_aabb(o), r) != inv) {
                        to_remove[n_remove++] = o;
                }
        }

        for (int n = 0; n < n_remove; n++) {
                objset_del(set, to_remove[n]);
        }
}

obj_listc_s obj_list_all(game_s *g)
{
        return objbucket_list(g, 0);
}

obj_listc_s objbucket_list(game_s *g, int bucketID)
{
        ASSERT(0 <= bucketID && bucketID < NUM_OBJ_BUCKETS);
        return objset_list(&g->objbuckets[bucketID].set);
}

void objbucket_copy_to_set(game_s *g, int bucketID, objset_s *set)
{
        ASSERT(0 <= bucketID && bucketID < NUM_OBJ_BUCKETS);
        *set = g->objbuckets[bucketID].set;
}

static inline void actor_step(game_s *g, obj_s *o, int sx, int sy)
{
        o->pos.x += sx;
        o->pos.y += sy;
        if (o->rope && o->ropenode) {
                v2_i32 d = {sx, sy};
                ropenode_move(g, o->rope, o->ropenode, d);
        }
}

static bool32 actor_try_wiggle(game_s *g, obj_s *o)
{
        rec_i32 aabb = obj_aabb(o);
        if (!room_area_blocked(g, aabb)) return 1;

        for (int y = -1; y <= +1; y++) {
                for (int x = -1; x <= +1; x++) {
                        rec_i32 r = translate_rec_xy(aabb, x, y);
                        if (!room_area_blocked(g, r)) {
                                actor_step(g, o, x, y);
                                return 1;
                        }
                }
        }

        o->squeezed = 1;
        if (o->onsqueeze) {
                o->onsqueeze(g, o);
        }
        return 0;
}

static bool32 actor_step_x(game_s *g, obj_s *o, int sx)
{
        ASSERT(abs_i(sx) <= 1);

        rec_i32 r = translate_rec_xy(obj_aabb(o), sx, 0);
        if (!room_area_blocked(g, r)) {
                actor_step(g, o, sx, 0);

                if (o->actorflags & ACTOR_FLAG_GLUE_GROUND) {
                        rec_i32 r1 = translate_rec_xy(obj_aabb(o), 0, +1);
                        rec_i32 r2 = translate_rec_xy(obj_aabb(o), 0, +2);
                        if (room_area_blocked(g, r1) == 0 &&
                            room_area_blocked(g, r2)) {
                                actor_step(g, o, 0, 1);
                        }
                }
                return 1;
        }

        // the position to the side is blocked
        // check if object can climb slopes and try it out
        if (o->actorflags & ACTOR_FLAG_CLIMB_SLOPES) {
                rec_i32 r1 = translate_rec_xy(obj_aabb(o), sx, -1);
                if (!room_area_blocked(g, r1)) {
                        actor_step(g, o, sx, -1);
                        return 1;
                }

                if (o->actorflags & ACTOR_FLAG_CLIMB_STEEP_SLOPES) {
                        rec_i32 r2 = translate_rec_xy(obj_aabb(o), sx, -2);
                        if (!room_area_blocked(g, r2)) {
                                actor_step(g, o, sx, -2);
                                return 1;
                        }
                }
        }

        return 0;
}

static bool32 actor_step_y(game_s *g, obj_s *o, int sy)
{
        ASSERT(abs_i(sy) <= 1);
        rec_i32 r = translate_rec_xy(obj_aabb(o), 0, sy);
        if (!room_area_blocked(g, r)) {
                actor_step(g, o, 0, sy);
                return 1;
        }
        return 0;
}

void actor_move(game_s *g, obj_s *o, int dx, int dy)
{
        for (int m = abs_i(dx), sx = sgn_i(dx); m > 0; m--) {
                if (!actor_step_x(g, o, sx)) {
                        o->vel_q8.x = 0;
                        break;
                }
        }

        for (int m = abs_i(dy), sy = sgn_i(dy); m > 0; m--) {
                if (!actor_step_y(g, o, sy)) {
                        o->vel_q8.y = 0;
                        break;
                }
        }
}

static void solid_step(game_s *g, obj_s *o, v2_i32 dt, obj_listc_s actors)
{
        ASSERT((abs_i(dt.x) == 1 && dt.y == 0) || (abs_i(dt.y) == 1 && dt.x == 0));

        hero_s *hero = (hero_s *)obj_get_tagged(g, OBJ_TAG_HERO);
        if (hero && hero->o.rope) {
                o->soliddisabled = 1;
                rope_moved_by_solid(g, hero->o.rope, o, dt);
                o->soliddisabled = 0;
        }

        o->pos    = v2_add(o->pos, dt);
        rec_i32 r = obj_aabb(o);

        for (int n = 0; n < actors.n; n++) {
                obj_s *a = actors.o[n];
                if (a->squeezed) continue;
                rec_i32 aabb  = obj_aabb(a);
                rec_i32 rfeet = translate_rec(obj_rec_bottom(a), dt);
                if (overlap_rec_excl(r, aabb) ||
                    overlap_rec_excl(r, rfeet) ||
                    obj_from_handle(a->linkedsolid) == o) {
                        if (dt.x != 0) actor_step_x(g, a, dt.x);
                        if (dt.y != 0) actor_step_y(g, a, dt.y);
                        actor_try_wiggle(g, a);
                }
        }
}

void solid_move(game_s *g, obj_s *o, int dx, int dy)
{
        obj_listc_s actors = objbucket_list(g, OBJ_BUCKET_ACTOR);
        v2_i32      vx     = {sgn_i(dx), 0};
        for (int m = abs_i(dx); m > 0; m--) {
                solid_step(g, o, vx, actors);
        }
        v2_i32 vy = {0, sgn_i(dy)};
        for (int m = abs_i(dy); m > 0; m--) {
                solid_step(g, o, vy, actors);
        }
}

bool32 solid_occupies(obj_s *solid, rec_i32 r)
{
        return overlap_rec_excl(obj_aabb(solid), r);
}

// apply gravity, drag, modify subposition and write pos_new
// uses subpixel position:
// subposition is [0, 255]. If the boundaries are exceeded
// the goal is to move a whole pixel left or right
void obj_apply_movement(obj_s *o)
{
        o->vel_q8      = v2_add(o->vel_q8, o->gravity_q8);
        o->vel_q8.x    = q_mulr(o->vel_q8.x, o->drag_q8.x, 8);
        o->vel_q8.y    = q_mulr(o->vel_q8.y, o->drag_q8.y, 8);
        o->vel_prev_q8 = o->vel_q8;
        o->subpos_q8   = v2_add(o->subpos_q8, o->vel_q8);
        o->tomove      = v2_add(o->tomove, v2_shr(o->subpos_q8, 8));
        o->subpos_q8.x &= 255;
        o->subpos_q8.y &= 255;
}