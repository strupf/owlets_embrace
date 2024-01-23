// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "obj.h"
#include "game.h"
#include "rope.h"

obj_handle_s obj_handle_from_obj(obj_s *o)
{
    obj_handle_s h = {0};
    h.o            = o;
    if (o)
        h.UID = o->UID;
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
    return (h.o && h.o->UID.u == h.UID.u);
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

    obj_UID_s UID = o->UID;
    *o            = (obj_s){0};
    o->UID        = UID;
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
        o->UID.gen++; // increase gen to devalidate existing handles
    } else {
        sys_printf("already deleted\n");
    }
}

bool32 obj_tag(game_s *g, obj_s *o, int tag)
{
    if (g->obj_tag[tag]) return 0;
    o->tags |= (flags32)1 << tag;
    g->obj_tag[tag] = o;
    return 1;
}

bool32 obj_untag(game_s *g, obj_s *o, int tag)
{
    if (g->obj_tag[tag] != o) return 0;
    o->tags &= ~((flags32)1 << tag);
    g->obj_tag[tag] = NULL;
    return 1;
}

obj_s *obj_get_tagged(game_s *g, int tag)
{
    return g->obj_tag[tag];
}

void objs_cull_to_delete(game_s *g)
{
    if (g->obj_ndelete <= 0) return;
    for (int n = 0; n < g->obj_ndelete; n++) {
        obj_s *o = g->obj_to_delete[n];

        for (int k = 0; k < NUM_OBJ_TAGS; k++) {
            if (g->obj_tag[k] == o)
                g->obj_tag[k] = NULL;
        }
        for (int k = 0; k < g->n_objrender; k++) {
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

#define WIGGLE_AMOUNT 2

    for (int y = -WIGGLE_AMOUNT; y <= +WIGGLE_AMOUNT; y++) {
        for (int x = -WIGGLE_AMOUNT; x <= +WIGGLE_AMOUNT; x++) {
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
    }
}

void squish_delete(game_s *g, obj_s *o)
{
    obj_delete(g, o);
}

static void actor_move_by(game_s *g, obj_s *o, v2_i32 dt)
{
    assert(o->flags & OBJ_FLAG_ACTOR);

    if (o->flags & OBJ_FLAG_PLATFORM) {
        platform_move(g, o, dt);
    } else {
        o->pos = v2_add(o->pos, dt);
    }

    if (o->ropenode) {
        ropenode_move(g, o->rope, o->ropenode, dt);
    }
}

void actor_move(game_s *g, obj_s *o, v2_i32 dt)
{
    assert(o->flags & OBJ_FLAG_ACTOR);

#define DO_BUMP_X                                                 \
    o->bumpflags |= sx > 0 ? OBJ_BUMPED_X_POS : OBJ_BUMPED_X_NEG; \
    break
#define DO_BUMP_Y                                                 \
    o->bumpflags |= sy > 0 ? OBJ_BUMPED_Y_POS : OBJ_BUMPED_Y_NEG; \
    break

    for (int m = abs_i(dt.x), sx = sgn_i(dt.x); m; m--) {
        rec_i32 aabb = obj_aabb(o);
        v2_i32  dtm  = {sx, 0};
        aabb.x += sx;

        if ((o->flags & OBJ_FLAG_CLAMP_TO_ROOM) &&
            (aabb.x + aabb.w > g->pixel_x || aabb.x < 0)) {
            DO_BUMP_X;
        }

        if (game_traversable(g, aabb)) {
            if ((o->moverflags & OBJ_MOVER_GLUE_GROUND) && 0 <= o->vel_q8.y) {
                rec_i32 a1 = aabb;
                a1.h += 1;

                if (game_traversable(g, a1)) {
                    rec_i32 a2 = aabb;
                    a2.h += 2;
                    if (game_traversable(g, a2)) {
                        rec_i32 a3 = aabb;
                        a3.h += 3;
                        if (!game_traversable(g, a3)) {
                            dtm.y = 2;
                        }
                    } else {
                        dtm.y = 1;
                    }
                }
            }
        } else if (o->moverflags & OBJ_MOVER_SLOPES) {
            aabb.y -= 1;
            dtm.y = -1;
            if (!game_traversable(g, aabb)) {
                DO_BUMP_X;
            }
        } else {
            DO_BUMP_X;
        }

        actor_move_by(g, o, dtm);
    }
    for (int m = abs_i(dt.y), sy = sgn_i(dt.y); 0 < m; m--) {
        rec_i32 aabb = obj_aabb(o);
        v2_i32  dtm  = {0, sy};
        aabb.y += sy;

        if ((o->flags & OBJ_FLAG_CLAMP_TO_ROOM) &&
            (aabb.y + aabb.h > g->pixel_y || aabb.y < 0)) {
            DO_BUMP_Y;
        }

        rec_i32 orec = 0 < sy ? obj_rec_bottom(o) : obj_rec_top(o);

        int collide_plat = 0;

        if ((o->moverflags & OBJ_MOVER_ONE_WAY_PLAT) && 0 < sy) {
            collide_plat = (orec.y & 15) == 0 && tile_one_way(g, orec);
            if (!collide_plat) {
                for (obj_each(g, a)) {
                    if (o == a) continue;
                    if (!(a->flags & OBJ_FLAG_PLATFORM)) continue;
                    rec_i32 rplat = {a->pos.x, a->pos.y, a->w, 1};
                    if (overlap_rec(orec, rplat)) {
                        collide_plat = 1;
                        break;
                    }
                }
            }
        }

        if (!collide_plat && game_traversable(g, orec)) {
            actor_move_by(g, o, dtm);
        } else if ((o->moverflags & OBJ_MOVER_AVOID_HEADBUMP) && sy < 0) {
            // jump corner correction
            // https://twitter.com/MaddyThorson/status/1238338578310000642
            for (int k = 1; k <= 4; k++) {
                rec_i32 recr = {aabb.x + k, aabb.y, aabb.w, aabb.h};
                rec_i32 recl = {aabb.x - k, aabb.y, aabb.w, aabb.h};
                if (game_traversable(g, recr)) {
                    actor_move_by(g, o, (v2_i32){+k, dtm.y});
                    goto CONTINUE_Y;
                }
                if (game_traversable(g, recl)) {
                    actor_move_by(g, o, (v2_i32){-k, dtm.y});
                    goto CONTINUE_Y;
                }
            }
            DO_BUMP_Y;
        } else {
            DO_BUMP_Y;
        }
    CONTINUE_Y:;
    }
}

static void platform_movestep(game_s *g, obj_s *o, v2_i32 dt)
{
    assert(v2_lensq(dt) == 1);

    rec_i32 rplat = {o->pos.x, o->pos.y, o->w, 1};
    o->pos        = v2_add(o->pos, dt);

    for (obj_each(g, a)) {
        if (a == o) continue;
        if (!(a->flags & OBJ_FLAG_ACTOR)) continue;
        if (!(a->moverflags & OBJ_MOVER_ONE_WAY_PLAT)) continue;
        bool32 linked = (obj_from_obj_handle(a->linked_solid) == o);

        rec_i32 feet = obj_rec_bottom(a);
        if (overlap_rec(feet, rplat))
            linked = 1;

        if (linked) {
            actor_move(g, a, dt);
        }
    }
}

void platform_move(game_s *g, obj_s *o, v2_i32 dt)
{
    assert(o->flags & OBJ_FLAG_PLATFORM);

    for (int m = abs_i(dt.x), sx = sgn_i(dt.x); m; m--) {
        v2_i32 dtx = {sx, 0};
        platform_movestep(g, o, dtx);
    }

    for (int m = abs_i(dt.y), sy = sgn_i(dt.y); m; m--) {
        v2_i32 dty = {0, sy};
        platform_movestep(g, o, dty);
    }
}

static void solid_movestep(game_s *g, obj_s *o, v2_i32 dt)
{
    assert(v2_lensq(dt) == 1);

    o->flags &= ~OBJ_FLAG_SOLID;
    for (int n = 0; n < g->n_ropes; n++) {
        rope_moved_by_solid(g, g->ropes[n], o, dt);
    }
    o->flags |= OBJ_FLAG_SOLID;

    rec_i32 aabbog = obj_aabb(o);
    o->pos         = v2_add(o->pos, dt);
    rec_i32 aabb   = obj_aabb(o);

    for (obj_each(g, a)) {
        if (!(a->flags & (OBJ_FLAG_ACTOR | OBJ_FLAG_PLATFORM))) continue;
        rec_i32 body   = obj_aabb(a);
        bool32  linked = (obj_from_obj_handle(a->linked_solid) == o);

        switch (a->ID) {
        case OBJ_ID_CRAWLER:
        case OBJ_ID_CRAWLER_SNAIL:
            if (overlap_rec_touch(body, aabbog))
                linked = 1;
            break;
        default: {
            rec_i32 feet = obj_rec_bottom(a);

            if (o->flags & OBJ_FLAG_SOLID) {
                if (overlap_rec(feet, aabbog))
                    linked = 1;
            }
            if (o->flags & OBJ_FLAG_PLATFORM) {
                rec_i32 rplat = {aabbog.x, aabbog.y, aabbog.w, 1};
                if (overlap_rec(feet, rplat))
                    linked = 1;
            }

        } break;
        }

        if (overlap_rec(body, aabb) || linked) {
            if (a->flags & OBJ_FLAG_PLATFORM)
                platform_move(g, a, dt);
            else
                actor_move(g, a, dt);
        }
    }
}

void solid_move(game_s *g, obj_s *o, v2_i32 dt)
{
    assert(o->flags & OBJ_FLAG_SOLID);

    for (int m = abs_i(dt.x), sx = sgn_i(dt.x); m; m--) {
        v2_i32 dtx = {sx, 0};
        solid_movestep(g, o, dtx);
    }

    for (int m = abs_i(dt.y), sy = sgn_i(dt.y); m; m--) {
        v2_i32 dty = {0, sy};
        solid_movestep(g, o, dty);
    }
}

void obj_interact(game_s *g, obj_s *o)
{
    switch (o->ID) {
    case OBJ_ID_SIGN: textbox_load_dialog(g, &g->textbox, o->filename); break;
    case OBJ_ID_SAVEPOINT: game_write_savefile(g); break;
    case OBJ_ID_SWITCH: switch_on_interact(g, o); break;
    case OBJ_ID_NPC: npc_on_interact(g, o); break;
    case OBJ_ID_DOOR_SWING: swingdoor_on_interact(g, o); break;
    }
}

// apply gravity, drag, modify subposition and write pos_new
// uses subpixel position:
// subposition is [0, 255]. If the boundaries are exceeded
// the goal is to move a whole pixel left or right
void obj_apply_movement(obj_s *o)
{
    o->vel_prev_q8 = o->vel_q8;
    o->vel_q8      = v2_add(o->vel_q8, o->gravity_q8);
    if (o->vel_cap_q8.x != 0)
        o->vel_q8.x = clamp_i(o->vel_q8.x, -o->vel_cap_q8.x, +o->vel_cap_q8.x);
    if (o->vel_cap_q8.y != 0)
        o->vel_q8.y = clamp_i(o->vel_q8.y, -o->vel_cap_q8.y, +o->vel_cap_q8.y);
    o->vel_q8.x = (o->vel_q8.x * o->drag_q8.x) >> 8;
    o->vel_q8.y = (o->vel_q8.y * o->drag_q8.y) >> 8;

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
    if (!game_traversable(g, rbot)) return 1;
    if ((o->flags & OBJ_FLAG_ACTOR) && (o->moverflags & OBJ_MOVER_ONE_WAY_PLAT)) {
        if (0 <= o->vel_q8.y && (rbot.y & 15) == 0 && tile_one_way(g, rbot))
            return 1;
        for (obj_each(g, k)) {
            if (k == o) continue;
            if (!(k->flags & OBJ_FLAG_PLATFORM)) continue;
            rec_i32 rplat = {k->pos.x, k->pos.y, k->w, 1};
            if (overlap_rec(rbot, rplat))
                return 1;
        }
    }
    return 0;
}

bool32 obj_would_fall_down_next(game_s *g, obj_s *o, int xdir)
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

obj_s *obj_savepoint_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_SAVEPOINT;
    o->flags |= OBJ_FLAG_INTERACTABLE;
    return o;
}

v2_i32 obj_constrain_to_rope(game_s *g, obj_s *o)
{
    if (!o->rope || !o->ropenode) return o->vel_q8;

    rope_s     *r          = o->rope;
    ropenode_s *rn         = o->ropenode;
    u32         len_q4     = rope_length_q4(g, r);
    u32         len_max_q4 = r->len_max_q4;
    if (len_q4 <= len_max_q4) return o->vel_q8; // rope is not stretched

    ropenode_s *rprev = rn->next ? rn->next : rn->prev;
    assert(rprev);

    v2_i32 ropedt    = v2_sub(rn->p, rprev->p);
    v2_i32 subpos_q4 = v2_shr(o->subpos_q8, 4);
    v2_i32 dt_q4     = v2_add(v2_shl(ropedt, 4), subpos_q4);

    // damping force
    v2_i32 fdamp = {0};
    v2_i32 vrad  = project_pnt_line(o->vel_q8, (v2_i32){0}, dt_q4);
    if (v2_dot(ropedt, o->vel_q8) > 0) {
        fdamp = v2_shr(v2_mul(vrad, 250), 8);
    }

    // spring force
    u32    dt_len         = len_q4 - len_max_q4;
    i32    fspring_scalar = (dt_len * 220) >> 8;
    // i32    fspring_scalar = pow2_i32(dt_len / 12);
    v2_i32 fspring        = v2_setlen(dt_q4, fspring_scalar);

    v2_i32 frope   = v2_add(fdamp, fspring);
    v2_i32 vel_new = v2_sub(o->vel_q8, frope);
    return vel_new;
}

int obj_health_change(obj_s *o, int dt)
{
    o->health = clamp_i(o->health + dt, 0, o->health_max);
    return o->health;
}

int enemy_obj_damage(obj_s *o, int dmg, int invincible_ticks)
{
    if (0 < o->invincible_tick) return o->health;
    int h = obj_health_change(o, -dmg);
    if (0 < h) {
        o->invincible_tick = invincible_ticks;
    }
    return h;
}

int obj_invincible_frame(obj_s *o)
{
    return ((o->invincible_tick >> 2) & 1);
}