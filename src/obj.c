// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "obj.h"
#include "game.h"
#include "rope.h"

obj_handle_s obj_handle_from_obj(obj_s *o)
{
    obj_handle_s h = {o, o ? o->generation : 0};
    return h;
}

obj_s *obj_from_obj_handle(obj_handle_s h)
{
    return (obj_handle_valid(h) ? h.o : 0);
}

bool32 obj_try_from_obj_handle(obj_handle_s h, obj_s **o_out)
{
    obj_s *o = obj_from_obj_handle(h);
    if (o_out) *o_out = o;
    return (o != 0);
}

bool32 obj_handle_valid(obj_handle_s h)
{
    return (h.o && h.o->generation == h.generation);
}

obj_s *obj_create(g_s *g)
{
    obj_s *o = g->obj_head_free;
    if (!o) {
        BAD_PATH
        return 0;
    }

    g->objrender_dirty              = 1;
    g->obj_render[g->n_objrender++] = o;
    g->obj_head_free                = o->next;

    u32 gen = o->generation;
    mclr(o, sizeof(obj_s));
    o->generation = gen;
    o->prev       = 0;
    o->next       = g->obj_head_busy;
    if (g->obj_head_busy) {
        g->obj_head_busy->prev = o;
    }
    g->obj_head_busy   = o;
    o->render_priority = RENDER_PRIO_DEFAULT_OBJ;
    o->on_squish       = obj_delete;
#if PLTF_DEBUG
    o->magic = OBJ_MAGIC;
#if 0
    static u32 n_warn = NUM_OBJ / 2;
    u32        n_pool = 0;
    for (obj_s *op = g->obj_head_free; op; op = op->next) {
        n_pool++;
    }
    if (n_pool < n_warn) {
        n_warn = n_pool;
        pltf_log("+++ OBJ POOL: ONLY %i LEFT!\n", n_warn);
    }
#endif
#endif
    return o;
}

void obj_delete(g_s *g, obj_s *o)
{
    if (!o) return;
    if (find_ptr_in_array(g->obj_to_delete, o, g->obj_ndelete) < 0) {
        g->obj_to_delete[g->obj_ndelete++] = o;
        o->generation++; // increase gen to devalidate existing handles
    } else {
        pltf_log("already deleted\n");
    }
}

void obj_handle_delete(g_s *g, obj_handle_s h)
{
    obj_delete(g, obj_from_obj_handle(h));
}

bool32 obj_tag(g_s *g, obj_s *o, i32 tag)
{
    if (g->obj_tag[tag]) return 0;
    g->obj_tag[tag] = o;
    return 1;
}

bool32 obj_untag(g_s *g, obj_s *o, i32 tag)
{
    if (g->obj_tag[tag] != o) return 0;
    g->obj_tag[tag] = 0;
    return 1;
}

obj_s *obj_get_tagged(g_s *g, i32 tag)
{
    return g->obj_tag[tag];
}

void objs_cull_to_delete(g_s *g)
{
    if (!g->obj_ndelete) return;

    for (u32 n = 0; n < g->obj_ndelete; n++) {
        obj_s *o = g->obj_to_delete[n];

        for (u32 k = 0; k < NUM_OBJ_TAGS; k++) {
            if (g->obj_tag[k] == o) {
                g->obj_tag[k] = 0;
            }
        }

        for (u32 k = 0; k < g->n_objrender; k++) {
            if (g->obj_render[k] == o) {
                g->obj_render[k] = g->obj_render[--g->n_objrender];
                break;
            }
        }

        if (o->next) {
            o->next->prev = o->prev;
        }
        if (o->prev) {
            o->prev->next = o->next;
        }
        if (g->obj_head_busy == o) {
            g->obj_head_busy = o->next;
        }

        o->next          = g->obj_head_free;
        g->obj_head_free = o;
    }
    g->obj_ndelete     = 0;
    g->objrender_dirty = 1;
}

bool32 obj_try_wiggle(g_s *g, obj_s *o)
{
    if (!(o->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS)) return 1;
    if (!(o->flags & OBJ_FLAG_ACTOR)) return 1;

    rec_i32 r = obj_aabb(o);
    if (!map_blocked_excl(g, r, o)) return 1;

    i32 nw = o->ID == OBJID_OWL ? 6 : 4;

    for (i32 n = 1; n <= nw; n++) {
        for (i32 yn = -n; yn <= +n; yn += n) {
            for (i32 xn = -n; xn <= +n; xn += n) {
                rec_i32 rr = {r.x + xn, r.y + yn, r.w, r.h};
                if (map_blocked_excl(g, rr, o)) continue;

                o->moverflags &= ~OBJ_MOVER_TERRAIN_COLLISIONS;
                obj_move(g, o, xn, yn);
                o->moverflags |= OBJ_MOVER_TERRAIN_COLLISIONS;
                return 1;
            }
        }
    }

    o->bumpflags |= OBJ_BUMP_SQUISH;
    return 0;
}

void squish_delete(g_s *g, obj_s *o)
{
    obj_delete(g, o);
}

void obj_move_by_q12(g_s *g, obj_s *o, i32 dx_q12, i32 dy_q12)
{
    o->subpos_q12.x += dx_q12;
    o->subpos_q12.y += dy_q12;
    i32 dx = o->subpos_q12.x >> 12;
    i32 dy = o->subpos_q12.y >> 12;
    o->subpos_q12.x &= 0xFFF;
    o->subpos_q12.y &= 0xFFF;

    obj_move(g, o, dx, dy);
}

void obj_move_by_v_q12(g_s *g, obj_s *o)
{
    o->subpos_q12 = v2_i32_add(o->subpos_q12, o->v_q12);
    i32 dx        = o->subpos_q12.x >> 12;
    i32 dy        = o->subpos_q12.y >> 12;
    o->subpos_q12.x &= 0xFFF;
    o->subpos_q12.y &= 0xFFF;

    obj_move(g, o, dx, dy);
}

void obj_v_q8_mul(obj_s *o, i32 mx_q8, i32 my_q8)
{
    obj_vx_q8_mul(o, mx_q8);
    obj_vy_q8_mul(o, my_q8);
}

void obj_vx_q8_mul(obj_s *o, i32 mx_q8)
{
    o->v_q12.x = shr_balanced_i32(i32_mul(o->v_q12.x, mx_q8), 8);
}

void obj_vy_q8_mul(obj_s *o, i32 my_q8)
{
    o->v_q12.y = shr_balanced_i32(i32_mul(o->v_q12.y, my_q8), 8);
}

void obj_v_q12_mul(obj_s *o, i32 mx_q12, i32 my_q12)
{
    obj_vx_q12_mul(o, mx_q12);
    obj_vy_q12_mul(o, my_q12);
}

void obj_vx_q12_mul(obj_s *o, i32 mx_q12)
{
    o->v_q12.x = shr_balanced_i32(i32_mul(o->v_q12.x, mx_q12), 12);
}

void obj_vy_q12_mul(obj_s *o, i32 my_q12)
{
    o->v_q12.y = shr_balanced_i32(i32_mul(o->v_q12.y, my_q12), 12);
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

rec_i32 obj_rec_x_leading(obj_s *o, i32 dx)
{
    return (0 <= dx ? obj_rec_right(o) : obj_rec_left(o));
}

rec_i32 obj_rec_y_leading(obj_s *o, i32 dy)
{
    return (0 <= dy ? obj_rec_bottom(o) : obj_rec_top(o));
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

bool32 obj_grounded(g_s *g, obj_s *o)
{
    v2_i32 offs = {0};
    return obj_grounded_at_offs(g, o, offs);
}

bool32 blocked_excl_offs(g_s *g, rec_i32 r, obj_s *o, i32 dx, i32 dy)
{
    if (o && !(o->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS)) return 0;

    rec_i32 ri = {r.x + dx, r.y + dy, r.w, r.h};
    if (tile_map_solid(g, ri)) return 1;

    for (obj_each(g, i)) {
        if (i != o && (i->flags & OBJ_FLAG_SOLID) && overlap_rec(r, obj_aabb(i))) {

            if (o && (o->flags & OBJ_FLAG_ACTOR) && obj_ignores_solid(o, i, 0))
                continue;
            return 1;
        }
    }
    return 0;
}

bool32 blocked_excl(g_s *g, rec_i32 r, obj_s *o)
{
    return blocked_excl_offs(g, r, o, 0, 0);
}

bool32 obj_grounded_at_offs(g_s *g, obj_s *o, v2_i32 offs)
{
    if (blocked_excl_offs(g, obj_aabb(o), o, offs.x, offs.y)) return 0;

    rec_i32 r = {o->pos.x + offs.x, o->pos.y + o->h + offs.y, o->w, 1};
    return (blocked_excl(g, r, o) || obj_on_platform(g, o, r.x, r.y, r.w));
}

bool32 obj_would_fall_down_next(g_s *g, obj_s *o, i32 xdir)
{
    if (!obj_grounded(g, o)) return 0;

    rec_i32 r1   = {o->pos.x + xdir, o->pos.y, o->w, o->h + 1};
    v2_i32  off1 = {xdir, 0};
    v2_i32  off2 = {xdir, 1};

    return (!map_blocked(g, r1) &&
            !obj_grounded_at_offs(g, o, off1) &&
            !obj_grounded_at_offs(g, o, off2));
}

enemy_s enemy_default()
{
    enemy_s e          = {0};
    e.sndID_die        = SNDID_ENEMY_DIE;
    e.sndID_hurt       = SNDID_ENEMY_HURT;
    e.hurt_tick_max    = 12;
    e.die_tick_max     = 5;
    e.coins_on_death   = 1;
    e.explode_on_death = 1;
    return e;
}

void enemy_hurt(g_s *g, obj_s *o, i32 dmg)
{
    if (!o->health) return;

    o->health = max_i32(0, (i32)o->health - dmg);

    if (o->health == 0) {
        g->enemies_killed++;
        o->on_update       = 0;
        o->enemy.hurt_tick = -o->enemy.die_tick_max;
        o->flags &= ~OBJ_FLAG_OWL_JUMPSTOMPABLE;
        o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
    } else {
        o->enemy.hurt_tick = o->enemy.hurt_tick_max;
    }

    o->enemy.flash_tick = 3;
    if (o->enemy.on_hurt) {
        o->enemy.on_hurt(g, o);
    }
}

bool32 obj_ignores_solid(obj_s *oactor, obj_s *osolid, i32 *index)
{
    if (!oactor) return 0;

    for (i32 n = 0; n < oactor->n_ignored_solids; n++) {
        if (obj_from_obj_handle(oactor->ignored_solids[n]) == osolid) {
            if (index) {
                *index = n;
            }
            return 1;
        }
    }
    return 0;
}

i32 obj_distsq(obj_s *a, obj_s *b)
{
    return v2_i32_distancesq(obj_pos_center(a), obj_pos_center(b));
}

i32 obj_dist_appr(obj_s *a, obj_s *b)
{
    return v2_i32_distance_appr(obj_pos_center(a), obj_pos_center(b));
}

bool32 obj_standing_on(obj_s *o_standing_on, obj_s *o_plat, i32 offx, i32 offy)
{
    if (!o_standing_on || !o_plat) return 0;

    rec_i32 rb = obj_rec_bottom(o_standing_on);
    rec_i32 rt = {o_plat->pos.x + offx, o_plat->pos.y + offy, o_plat->w, 1};
    return overlap_rec(rb, rt);
}