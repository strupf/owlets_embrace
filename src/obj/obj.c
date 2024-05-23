// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "obj.h"
#include "game.h"
#include "rope.h"

obj_handle_s obj_handle_from_obj(obj_s *o)
{
    obj_handle_s h = {0};
    if (o) {
        h.o   = o;
        h.GID = o->GID;
    }
    return h;
}

obj_s *obj_from_obj_handle(obj_handle_s h)
{
    return (obj_handle_valid(h) ? h.o : NULL);
}

bool32 obj_try_from_obj_handle(obj_handle_s h, obj_s **o_out)
{
    obj_s *o = obj_from_obj_handle(h);
    if (o_out) *o_out = o;
    return (o != NULL);
}

bool32 obj_handle_valid(obj_handle_s h)
{
    return (h.o && h.o->GID == h.GID);
}

obj_s *obj_create(game_s *g)
{
    assert(g->obj_head_free);
    obj_s *o = g->obj_head_free;

    if (!o) {
        BAD_PATH
        return NULL;
    }
    g->objrender_dirty              = 1;
    g->obj_render[g->n_objrender++] = o;
    g->obj_head_free                = o->next;

    u32 GID = o->GID;
    *o      = (obj_s){0};
    o->GID  = GID;
#ifdef SYS_DEBUG
    o->magic = OBJ_MAGIC;
#endif
    o->next          = g->obj_head_busy;
    g->obj_head_busy = o;
    return o;
}

void obj_delete(game_s *g, obj_s *o)
{
    if (!o) return;
    if (ptr_index_in_arr(g->obj_to_delete, o, g->obj_ndelete) < 0) {
        g->obj_to_delete[g->obj_ndelete++] = o;

        o->GID = obj_GID_incr_gen(o->GID); // increase gen to devalidate existing handles
    } else {
        sys_printf("already deleted\n");
    }
}

bool32 obj_tag(game_s *g, obj_s *o, i32 tag)
{
    if (g->obj_tag[tag]) return 0;
    o->tags |= (flags32)1 << tag;
    g->obj_tag[tag] = o;
    return 1;
}

bool32 obj_untag(game_s *g, obj_s *o, i32 tag)
{
    if (g->obj_tag[tag] != o) return 0;
    o->tags &= ~((flags32)1 << tag);
    g->obj_tag[tag] = NULL;
    return 1;
}

obj_s *obj_get_tagged(game_s *g, i32 tag)
{
    return g->obj_tag[tag];
}

void objs_cull_to_delete(game_s *g)
{
    if (g->obj_ndelete <= 0) return;
    for (i32 n = 0; n < g->obj_ndelete; n++) {
        obj_s *o = g->obj_to_delete[n];

        for (i32 k = 0; k < NUM_OBJ_TAGS; k++) {
            if (g->obj_tag[k] == o)
                g->obj_tag[k] = NULL;
        }
        for (i32 k = 0; k < g->n_objrender; k++) {
            if (g->obj_render[k] == o) {
                g->obj_render[k] = g->obj_render[--g->n_objrender];
                break;
            }
        }

        if (g->obj_head_busy == o) {
            g->obj_head_busy = o->next;
        } else {
            for (obj_each(g, ot)) {
                if (ot->next == o) {
                    ot->next = o->next;
                    break;
                }
            }
        }

        o->next          = g->obj_head_free;
        g->obj_head_free = o;
    }
    g->obj_ndelete     = 0;
    g->objrender_dirty = 1;
}

bool32 actor_try_wiggle(game_s *g, obj_s *o)
{
    rec_i32 r = obj_aabb(o);
    if (game_traversable(g, r)) return 1;

    i32 wiggle = o->ID == OBJ_ID_HERO ? 16 : 4;

    for (i32 y = -wiggle; y <= +wiggle; y++) {
        for (i32 x = -wiggle; x <= +wiggle; x++) {
            rec_i32 rr = {r.x + x, r.y + y, r.w, r.h};
            if (game_traversable(g, rr)) {
                o->pos.x += x;
                o->pos.y += y;
                return 1;
            }
        }
    }

    o->bumpflags |= OBJ_BUMPED_SQUISH;
    obj_on_squish(g, o);
    return 0;
}

void obj_on_squish(game_s *g, obj_s *o)
{
    switch (o->ID) {
    default:
        obj_delete(g, o);
        sys_printf("Squishkill\n");
        break;
    case OBJ_ID_HERO:
        hero_on_squish(g, o);
        break;
    case OBJ_ID_HOOK: {
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        assert(ohero);
        hook_destroy(g, ohero, o);
        break;
    }
    }
}

void squish_delete(game_s *g, obj_s *o)
{
    obj_delete(g, o);
}

static bool32 map_blocked_by_solid(game_s *g, obj_s *o, rec_i32 r, i32 m)
{
    for (obj_each(g, it)) {
        if (it == o) continue;
        if (it->mass == 0) continue;
        if (m <= it->mass && overlap_rec(r, obj_aabb(it))) {
            obj_s *ocarry = (o && o->ID == OBJ_ID_HERO) ? carryable_present(g) : NULL;
            if (ocarry == it) continue;
            return 1;
        }
    }
    return 0;
}

static bool32 map_blocked_by_any_solid(game_s *g, rec_i32 r)
{
    for (obj_each(g, it)) {
        if (it->mass == 0) continue;
        if (overlap_rec(r, obj_aabb(it))) {
            return 1;
        }
    }
    return 0;
}

static bool32 map_blocked_by_any_solid_pt(game_s *g, i32 x, i32 y)
{
    v2_i32 pt = {x, y};
    for (obj_each(g, it)) {
        if (it->mass == 0) continue;
        if (overlap_rec_pnt(obj_aabb(it), pt)) {
            return 1;
        }
    }
    return 0;
}

bool32 map_blocked(game_s *g, obj_s *o, rec_i32 r, i32 m)
{
    if (tile_map_solid(g, r)) return 1;
    return (map_blocked_by_solid(g, o, r, m));
}

bool32 map_blocked_pt(game_s *g, obj_s *o, i32 x, i32 y, i32 m)
{
    if (tile_map_solid_pt(g, x, y)) return 0;
    rec_i32 r = {x, y, 1, 1};
    return (map_blocked_by_solid(g, NULL, r, 0));
}

bool32 game_traversable(game_s *g, rec_i32 r)
{
    if (tile_map_solid(g, r)) return 0;
    if (map_blocked_by_any_solid(g, r)) return 0;
    return 1;
}

bool32 game_traversable_pt(game_s *g, i32 x, i32 y)
{
    if (tile_map_solid_pt(g, x, y)) return 0;
    if (map_blocked_by_any_solid_pt(g, x, y)) return 0;
    return 1;
}

bool32 solid_overlaps_mass_eq_or_higher(game_s *g, rec_i32 r, i32 m)
{
    return map_blocked(g, NULL, r, m);
}

bool32 obj_step_internal(game_s *g, obj_s *o, i32 dx, i32 dy, i32 m)
{
    v2_i32 dt = {dx, dy};
    if (0 < o->mass && g->hero_mem.rope_active) {
        rope_moved_by_solid(g, &g->hero_mem.rope, o, dt);
    }

    rec_i32 cr     = obj_aabb(o);
    obj_s  *ocarry = (o->ID == OBJ_ID_HERO ? carryable_present(g) : NULL);
    if (ocarry) {     // actively move the hero, and move the solid with it
        if (dy < 0) { // upwards -> move solid first
            if (!obj_step_y(g, ocarry, -1, 0, ocarry->mass + 1)) {
                o->bumpflags |= OBJ_BUMPED_Y_NEG;
                return 0;
            }
            o->pos.y--;
        }
        if (dy > 0) { // downwards -> move hero first
            o->pos.y++;
            if (!obj_step_y(g, ocarry, +1, 0, 0)) {
                return 0;
            }
        }

        if (dx) {
            if (!obj_step_x(g, ocarry, dx, 0, 0)) {
                o->bumpflags |= (0 < dx ? OBJ_BUMPED_X_POS : OBJ_BUMPED_X_NEG);
                return 0;
            }
            o->pos.x += dx;
        }
    } else {
        o->pos.x += dx;
        o->pos.y += dy;
    }

    if (o->ropenode) {
        ropenode_move(g, o->rope, o->ropenode, dt);
    }

    if (o->mass <= 0) return 1;

    // solid pushing
    const rec_i32 oaabb = obj_aabb(o);
    for (obj_each(g, it)) {
        if (o == it) continue;
        bool32 linked  = 0;
        bool32 pushed  = 0;
        bool32 carried = 0;

        switch (it->ID) {
        case OBJ_ID_CRAWLER_CATERPILLAR:
        case OBJ_ID_CRAWLER:
            if (overlap_rec_touch(obj_aabb(it), cr))
                linked = 1;
            break;
        }

        if (it->mass < o->mass) {
            pushed = overlap_rec(oaabb, obj_aabb(it));
        }
        if (it->mass <= o->mass) {
            carried = overlap_rec(cr, obj_rec_bottom(it));
        }
        if (linked || pushed || carried) {
            obj_step_x(g, it, dx, 1, pushed ? m : 0);
            obj_step_y(g, it, dy, 1, pushed ? m : 0);
        }
    }
    return 1;
}

bool32 obj_step_x(game_s *g, obj_s *o, i32 dx, bool32 slide, i32 mpush)
{
    if (dx == 0) return 0;
    i32 m = (0 < o->mass && mpush) ? mpush : o->mass;

    rec_i32 checkr = dx == 1 ? obj_rec_right(o) : obj_rec_left(o);

    // try side stepping
    if ((o->moverflags & OBJ_MOVER_MAP) && map_blocked(g, o, checkr, m)) {
        if (!slide) {
            o->bumpflags |= dx == 1 ? OBJ_BUMPED_X_POS : OBJ_BUMPED_X_NEG;
            return 0;
        }

        rec_i32 r1       = {o->pos.x + dx, o->pos.y - 1, o->w, o->h};
        rec_i32 r2       = {o->pos.x + dx, o->pos.y + 1, o->w, o->h};
        bool32  could_r1 = (o->moverflags & OBJ_MOVER_SLIDE_Y_NEG) && !map_blocked(g, o, r1, m);
        bool32  could_r2 = (o->moverflags & OBJ_MOVER_SLIDE_Y_POS) && !map_blocked(g, o, r2, m);

        if (!(could_r1 && obj_step_y(g, o, -1, 0, m)) &&
            !(could_r2 && obj_step_y(g, o, +1, 0, m))) {
            o->bumpflags |= dx == 1 ? OBJ_BUMPED_X_POS : OBJ_BUMPED_X_NEG;
            return 0;
        }
    }

    checkr = dx == 1 ? obj_rec_right(o) : obj_rec_left(o);

    if ((o->moverflags & OBJ_MOVER_MAP) && map_blocked(g, o, checkr, m)) {
        o->bumpflags |= dx == 1 ? OBJ_BUMPED_X_POS : OBJ_BUMPED_X_NEG;
        return 0;
    }

    if (!obj_step_internal(g, o, dx, 0, m)) return 0;

    if (o->moverflags & OBJ_MOVER_GLUE_GROUND) {
        rec_i32 rg1 = obj_rec_bottom(o);
        rec_i32 rg2 = {rg1.x, rg1.y + 1, rg1.w, rg1.h};
        if (!map_blocked(g, o, rg1, o->mass) && map_blocked(g, o, rg2, o->mass)) {
            obj_step_y(g, o, +1, 0, 0);
        }
    }

    return 1;
}

bool32 obj_step_y(game_s *g, obj_s *o, i32 dy, bool32 slide, i32 mpush)
{
    if (dy == 0) return 0;
    i32 m = (0 < o->mass && mpush) ? mpush : o->mass;

    rec_i32 checkr = dy == 1 ? obj_rec_bottom(o) : obj_rec_top(o);

    if ((o->moverflags & OBJ_MOVER_MAP) && map_blocked(g, o, checkr, m)) {
        if (!slide) {
            o->bumpflags |= dy == 1 ? OBJ_BUMPED_Y_POS : OBJ_BUMPED_Y_NEG;
            return 0;
        }
        rec_i32 r1       = {o->pos.x - 1, o->pos.y + dy, o->w, o->h};
        rec_i32 r2       = {o->pos.x + 1, o->pos.y + dy, o->w, o->h};
        bool32  could_r1 = (o->moverflags & OBJ_MOVER_SLIDE_X_NEG) && !map_blocked(g, o, r1, m);
        bool32  could_r2 = (o->moverflags & OBJ_MOVER_SLIDE_X_POS) && !map_blocked(g, o, r2, m);

        if (!(could_r1 && obj_step_x(g, o, -1, 0, m)) &&
            !(could_r2 && obj_step_x(g, o, +1, 0, m))) {
            o->bumpflags |= dy == 1 ? OBJ_BUMPED_Y_POS : OBJ_BUMPED_Y_NEG;
            return 0;
        }
    }

    checkr = dy == 1 ? obj_rec_bottom(o) : obj_rec_top(o);

    if ((o->moverflags & OBJ_MOVER_MAP) && map_blocked(g, o, checkr, m)) {
        o->bumpflags |= dy == 1 ? OBJ_BUMPED_Y_POS : OBJ_BUMPED_Y_NEG;
        return 0;
    }

    if (!obj_step_internal(g, o, 0, dy, m)) return 0;

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        rec_i32 rhero = obj_rec_bottom(ohero);
        if (ohero == o && 0 < dy) {
            for (obj_each(g, it)) {
                if (!(it->flags & OBJ_FLAG_CAN_BE_JUMPED_ON)) continue;
                rec_i32 ro = {it->pos.x, it->pos.y, it->w, 1};
                if (!overlap_rec(rhero, ro)) continue;
                it->bumpflags |= OBJ_BUMPED_JUMPED_ON;
                ohero->bumpflags |= OBJ_BUMPED_ON_HEAD;
            }
        } else if ((o->flags & OBJ_FLAG_CAN_BE_JUMPED_ON) && dy < 0) {
            rec_i32 ro = {o->pos.x, o->pos.y, o->w, 1};
            if (overlap_rec(rhero, ro)) {
                o->bumpflags |= OBJ_BUMPED_JUMPED_ON;
                ohero->bumpflags |= OBJ_BUMPED_ON_HEAD;
            }
        }
    }

    return 1;
}

void obj_move(game_s *g, obj_s *o, v2_i32 dt)
{
    for (i32 m = abs_i(dt.x), sx = sgn_i(dt.x); 0 < m; m--) {
        obj_step_x(g, o, sx, 1, 0);
    }

    for (i32 m = abs_i(dt.y), sy = sgn_i(dt.y); 0 < m; m--) {
        obj_step_y(g, o, sy, 1, 0);
    }
}

enum {
    ACTOR_MOVE_SUCCESS,
    ACTOR_MOVE_BUMPED = 1 << 0,
    ACTOR_MOVE_DECR_M = 1 << 1,
};

// apply gravity, drag, modify subposition and write pos_new
// uses subpixel position:
// subposition is [0, 255]. If the boundaries are exceeded
// the goal is to move a whole pixel left or right
void obj_apply_movement(obj_s *o)
{
    o->vel_prev_q8 = o->vel_q8;
    o->vel_q8      = v2_add(o->vel_q8, o->gravity_q8);
    o->vel_q8.x    = (o->vel_q8.x * o->drag_q8.x) >> 8;
    o->vel_q8.y    = (o->vel_q8.y * o->drag_q8.y) >> 8;
    if (o->vel_cap_q8.x != 0)
        o->vel_q8.x = clamp_i(o->vel_q8.x, -o->vel_cap_q8.x, +o->vel_cap_q8.x);
    if (o->vel_cap_q8.y != 0)
        o->vel_q8.y = clamp_i(o->vel_q8.y, -o->vel_cap_q8.y, +o->vel_cap_q8.y);

    o->subpos_q8 = v2_add(o->subpos_q8, o->vel_q8);
    o->tomove    = v2_add(o->tomove, v2_shr(o->subpos_q8, 8));
    o->subpos_q8.x &= 255;
    o->subpos_q8.y &= 255;
}

bool32 overlap_obj(obj_s *a, obj_s *b)
{
    return (overlap_rec(obj_aabb(a), obj_aabb(b)));
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

v2_i32 obj_pos_center(obj_s *o)
{
    v2_i32 p = {o->pos.x + (o->w >> 1), o->pos.y + (o->h >> 1)};
    return p;
}

v2_i32 obj_pos_bottom_center(obj_s *o)
{
    v2_i32 p = {o->pos.x + (o->w >> 1), o->pos.y + o->h};
    return p;
}

bool32 obj_grounded(game_s *g, obj_s *o)
{
    v2_i32 offs = {0};
    return obj_grounded_at_offs(g, o, offs);
}

bool32 obj_grounded_at_offs(game_s *g, obj_s *o, v2_i32 offs)
{
    rec_i32 rbot = obj_rec_bottom(o);
    rbot.x += offs.x;
    rbot.y += offs.y;

    if (map_blocked(g, o, rbot, o->mass)) return 1;
    if ((o->flags & OBJ_FLAG_ACTOR) && (o->moverflags & OBJ_MOVER_ONE_WAY_PLAT)) {
        if (0 <= o->vel_q8.y && (rbot.y & 15) == 0 && tile_map_one_way(g, rbot))
            return 1;
        for (obj_each(g, k)) {
            if (k == o) continue;
            if (!(k->flags & OBJ_FLAG_PLATFORM)) continue;
            if ((k->flags & OBJ_FLAG_PLATFORM_HERO_ONLY) &&
                o->ID != OBJ_ID_HERO) continue;
            rec_i32 rplat = {k->pos.x, k->pos.y, k->w, 1};
            if (overlap_rec(rbot, rplat))
                return 1;
        }
    }
    return 0;
}

bool32 obj_would_fall_down_next(game_s *g, obj_s *o, i32 xdir)
{
    if (!obj_grounded(g, o)) return 0;

    rec_i32 r1   = {o->pos.x + xdir, o->pos.y, o->w, o->h + 1};
    v2_i32  off1 = {xdir, 0};
    v2_i32  off2 = {xdir, 1};

    return (game_traversable(g, r1) &&
            !obj_grounded_at_offs(g, o, off1) &&
            !obj_grounded_at_offs(g, o, off2));
}

obj_s *obj_closest_interactable(game_s *g, v2_i32 pos)
{
    u32    interactable_dt = pow2_i32(INTERACTABLE_DIST); // max distance
    obj_s *interactable    = NULL;
    for (obj_each(g, o)) {
        if (!(o->flags & OBJ_FLAG_INTERACTABLE)) continue;
        u32 d = v2_distancesq(pos, o->pos);
        if (d < interactable_dt) {
            interactable_dt = d;
            interactable    = o;
        }
    }
    return interactable;
}

v2_i32 obj_constrain_to_rope(game_s *g, obj_s *o)
{
    if (!o->rope || !o->ropenode) return o->vel_q8;

    rope_s     *r          = o->rope;
    ropenode_s *rn         = o->ropenode;
    i32         len_q4     = rope_length_q4(g, r);
    i32         len_max_q4 = r->len_max_q4;
    i32         dt_len     = len_q4 - len_max_q4;
    if (dt_len <= 0) return o->vel_q8; // rope is not stretched

    ropenode_s *rprev = rn->next ? rn->next : rn->prev;
    assert(rprev);

    v2_i32 ropedt = v2_sub(rn->p, rprev->p);
    v2_i32 dt_q4  = v2_add(v2_shl(ropedt, 4), v2_shr(o->subpos_q8, 4));

    // damping force

    v2_i32 fdamp = {0};
    if (v2_dot(ropedt, o->vel_q8) > 0) {
        v2_i32 vrad = project_pnt_line(o->vel_q8, (v2_i32){0}, dt_q4);
        fdamp       = v2_mulq(vrad, 250, 8);
    }

    // spring force
    i32    fspring_scalar = (dt_len * 250) >> 8;
    v2_i32 fspring        = v2_setlen(dt_q4, fspring_scalar);

    v2_i32 frope   = v2_add(fdamp, fspring);
    v2_i32 vel_new = v2_sub(o->vel_q8, frope);
    return vel_new;
}

enemy_s enemy_default()
{
    enemy_s e    = {0};
    e.sndID_die  = SNDID_ENEMY_DIE;
    e.sndID_hurt = SNDID_ENEMY_HURT;
    return e;
}

obj_s *carryable_present(game_s *g)
{
    obj_s *ohero  = obj_get_tagged(g, OBJ_TAG_HERO);
    obj_s *ocarry = obj_get_tagged(g, OBJ_TAG_CARRIED);
    if (!ohero || !ocarry) return 0;

    v2_i32 pos = carryable_pos_on_hero(ohero, ocarry, NULL);
    if (v2_eq(pos, ocarry->pos)) return ocarry;
    carryable_on_drop(g, ocarry);
    return NULL;
}

v2_i32 carryable_pos_on_hero(obj_s *ohero, obj_s *ocarry, rec_i32 *rlift)
{
    rec_i32 rhero = obj_aabb(ohero);
    v2_i32  pos   = {rhero.x + rhero.w / 2 - ocarry->w / 2,
                     rhero.y - ocarry->h};
    if (rlift) {
        rec_i32 rcarr = {pos.x, pos.y, ocarry->w, ocarry->h};
        *rlift        = rcarr;
    }
    return pos;
}

void carryable_on_lift(game_s *g, obj_s *o)
{
    assert(o->flags & OBJ_FLAG_CARRYABLE);

    o->flags &= ~OBJ_FLAG_CARRYABLE;
    o->flags &= ~OBJ_FLAG_MOVER;
    o->tomove.x   = 0;
    o->tomove.y   = 0;
    o->carry.tick = 1;
    o->carry.time = 1;

    switch (o->ID) {
    case 1000: {
        box_on_lift(g, o);
        break;
    }
    }
}

void carryable_on_drop(game_s *g, obj_s *o)
{
    assert(!(o->flags & OBJ_FLAG_CARRYABLE));
    obj_untag(g, o, OBJ_TAG_CARRIED);
    hero_s *h   = &g->hero_mem;
    h->carrying = 0;
    o->flags |= OBJ_FLAG_CARRYABLE;
    o->flags |= OBJ_FLAG_MOVER;

    switch (o->ID) {
    case 1000: {
        box_on_drop(g, o);
        break;
    }
    }
}

#define CARRYABLE_PICKUP_TICKS 4 // half the time

v2_i32 carryable_animate_spr_offset(obj_s *o)
{
    v2_i32 vz = {0};
    if (o->carry.tick) {
        o->carry.tick++;
        if ((CARRYABLE_PICKUP_TICKS << 1) <= o->carry.tick) {
            o->carry.tick = 0;
            return vz;
        }
        v2_i32 p1 = {o->carry.offs.x, o->carry.offs.y >> 2};
        if (o->carry.tick <= CARRYABLE_PICKUP_TICKS) {
            return v2_lerp(o->carry.offs, p1, o->carry.tick, CARRYABLE_PICKUP_TICKS);
        } else {
            return v2_lerp(p1, vz, o->carry.tick - CARRYABLE_PICKUP_TICKS, CARRYABLE_PICKUP_TICKS);
        }
    }
    return vz;
}