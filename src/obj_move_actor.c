// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

b32  obj_step_actor(g_s *g, obj_s *o, i32 sx, i32 sy);
b32  obj_actor_blocked(g_s *g, obj_s *o, rec_i32 r, i32 sx, i32 sy);
void obj_step_actor_commit(g_s *g, obj_s *o, i32 sx, i32 sy);

void obj_move_actor(g_s *g, obj_s *o, i32 dx, i32 dy)
{
    if (o->ID == OBJID_HERO && dx) {
        // pltf_log("walk: %i\n", abs_i32(dx));
    }
    for (i32 m = abs_i32(dx), s = 0 < dx ? +1 : -1; m; m--) {
        b32 was_grounded = obj_grounded(g, o);
        if (!obj_step_actor(g, o, s, 0)) break;
        // glue ground conditions
        if ((o->moverflags & OBJ_MOVER_GLUE_GROUND) &&
            was_grounded &&
            !obj_grounded(g, o) &&
            obj_grounded_at_offs(g, o, CINIT(v2_i32){0, 1})) {
            obj_step_actor(g, o, 0, +1);
        }
    }
    for (i32 m = abs_i32(dy), s = 0 < dy ? +1 : -1; m; m--) {
        if (!obj_step_actor(g, o, 0, s)) break;
    }
}

b32 obj_step_actor(g_s *g, obj_s *o, i32 sx, i32 sy)
{
    assert(!(o->flags & OBJ_FLAG_SOLID));
    assert((abs_i32(sx) <= 1 && sy == 0) || (abs_i32(sy) <= 1 && sx == 0));
    if (sx == 0 && sy == 0) return 1;

    rec_i32 r = obj_aabb(o);

    if (!obj_actor_blocked(g, o, r, sx, sy)) {
        obj_step_actor_commit(g, o, sx, sy);
        return 1;
    }

    if (sx) {
        // see if obj can move in y first, and then in x (diagonal slide)
        if ((o->moverflags & OBJ_MOVER_SLIDE_Y_NEG) &&
            !obj_actor_blocked(g, o, r, +0, -1) &&
            !obj_actor_blocked(g, o, r, sx, -1)) {
            obj_step_actor_commit(g, o, +0, -1);
            obj_step_actor_commit(g, o, sx, +0);
            return 1;
        }
        if ((o->moverflags & OBJ_MOVER_SLIDE_Y_NEG) &&
            !obj_actor_blocked(g, o, r, +0, +1) &&
            !obj_actor_blocked(g, o, r, sx, +1)) {
            obj_step_actor_commit(g, o, +0, +1);
            obj_step_actor_commit(g, o, sx, +0);
            return 1;
        }
        o->bumpflags |= obj_bump_x_flag(sx);
    }

    if (sy) {
        // see if obj can move in x first, and then in y (diagonal slide)
        if ((o->moverflags & OBJ_MOVER_SLIDE_X_NEG) &&
            !obj_actor_blocked(g, o, r, -1, +0) &&
            !obj_actor_blocked(g, o, r, -1, sy)) {
            obj_step_actor_commit(g, o, -1, +0);
            obj_step_actor_commit(g, o, +0, sy);
            return 1;
        }
        if ((o->moverflags & OBJ_MOVER_SLIDE_X_NEG) &&
            !obj_actor_blocked(g, o, r, +1, +0) &&
            !obj_actor_blocked(g, o, r, +1, sy)) {
            obj_step_actor_commit(g, o, +1, +0);
            obj_step_actor_commit(g, o, +0, sy);
            return 1;
        }
        // avoid headbump = try move around solid corner
        i32 n_hb = ((o->moverflags & OBJ_MOVER_AVOID_HEADBUMP) && sy < 0)
                       ? 12
                       : 0;
        for (i32 n = 1; n <= n_hb; n++) {
            for (i32 s = -1; s <= +1; s += 2) {
                i32 sn = s * n;
                if (!obj_actor_blocked(g, o, r, sn, +0) &&
                    !obj_actor_blocked(g, o, r, sn, -1)) {
                    for (i32 i = 0; i < n; i++) {
                        obj_step_actor_commit(g, o, s, 0);
                    }
                    obj_step_actor_commit(g, o, 0, -1);
                    return 1;
                }
            }
        }
        o->bumpflags |= obj_bump_y_flag(sy);
    }
    return 0;
}

b32 obj_actor_blocked(g_s *g, obj_s *o, rec_i32 r, i32 sx, i32 sy)
{
    if (obj_step_is_clamped(g, o, sx, sy)) return 1;
    rec_i32 ro = {r.x + sx, r.y + sy, r.w, r.h};
    if ((o->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS) && blocked_excl(g, ro, o)) {
        return 1;
    }

    bool32 coll_platform = 0;
    if ((o->moverflags & OBJ_MOVER_ONE_WAY_PLAT) && 0 < sy) {
        // check tiles for platform
        rec_i32 rb = {r.x + sx, r.y + r.h - 1 + sy, r.w, 1};
        rec_i32 rg = {0, 0, g->pixel_x, g->pixel_y};
        rec_i32 ri;
        if ((rb.y & 15) == 0 && intersect_rec(rb, rg, &ri)) {
            i32 ty  = (ri.y) >> 4;
            i32 tx0 = (ri.x) >> 4;
            i32 tx1 = (ri.x + ri.w - 1) >> 4;

            for (i32 tx = tx0; tx <= tx1; tx++) {
                switch (g->tiles[tx + ty * g->tiles_x].collision) {
                case TILE_ONE_WAY:
                case TILE_LADDER_ONE_WAY:
                    coll_platform = 1;
                    goto BREAKTXLOOP;
                }
            }
        BREAKTXLOOP:;
        }

        // check objects for platform
        for (obj_each(g, i)) {
            if (i == o) continue;

            rec_i32 rplat = {i->pos.x, i->pos.y, i->w, 1};
            if (overlap_rec(ro, rplat)) {
                if ((i->flags & OBJ_FLAG_PLATFORM))
                    coll_platform |= 1;
                coll_platform |= (o->ID == OBJID_HERO) &&
                                 (i->flags & OBJ_FLAG_HERO_PLATFORM);
            }
        }
    }

    if (o->ID == OBJID_HERO && 0 < sy) {
        // check hero jumping on objects
        rec_i32 rstomp = {ro.x - HERO_W_STOMP_ADD_SYMM,
                          ro.y + ro.h - 1,
                          ro.w + HERO_W_STOMP_ADD_SYMM * 2,
                          1};

        for (obj_each(g, i)) {
            if (i == o || !(i->flags & OBJ_FLAG_HERO_JUMPSTOMPABLE))
                continue;
            rec_i32 rplat = {i->pos.x, i->pos.y, i->w, 1};
            if (overlap_rec(rstomp, rplat)) {
                if (hero_stomping(o) && (i->flags & OBJ_FLAG_HERO_STOMPABLE)) {
                    coll_platform = 1;
                    hero_register_jumpstomped(o, i, 1);
                }
                if ((i->flags & OBJ_FLAG_HERO_JUMPABLE)) {
                    coll_platform = 1;
                    hero_register_jumpstomped(o, i, 0);
                }
            }
        }
    }

    if (coll_platform) {
        return 1;
    }
    return 0;
}

void obj_step_actor_commit(g_s *g, obj_s *o, i32 sx, i32 sy)
{
    assert(!(o->flags & OBJ_FLAG_SOLID));
    assert((abs_i32(sx) == 1 && sy == 0) || (abs_i32(sy) == 1 && sx == 0));

    if (o->flags & OBJ_FLAG_PLATFORM_ANY) {
        u64 flagp = (o->flags & OBJ_FLAG_PLATFORM_ANY);
        o->flags &= ~OBJ_FLAG_PLATFORM_ANY;
        rec_i32 rplat = {o->pos.x, o->pos.y, o->w, 1};
        for (obj_each(g, i)) {
            if (!(i->flags & OBJ_FLAG_ACTOR)) continue;
            if (!(i->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS)) continue;
            if (!(i->moverflags & OBJ_MOVER_ONE_WAY_PLAT)) continue;
            if (!overlap_rec(rplat, obj_rec_bottom(i))) continue;

            // check if stands on platform
            if ((((flagp & OBJ_FLAG_HERO_PLATFORM) && i->ID == OBJID_HERO) ||
                 ((flagp & OBJ_FLAG_PLATFORM)))) {
                obj_step_actor(g, i, sx, +0);
                obj_step_actor(g, i, +0, sy);
            }
        }
        o->flags |= flagp;
    }

    // check if hero is now jumping on this entity
    obj_s *ohero = obj_get_hero(g);
    if ((o->flags & OBJ_FLAG_HERO_JUMPSTOMPABLE) && ohero && sy < 0) {
        rec_i32 rplat  = {o->pos.x, o->pos.y, o->w, 1};
        rec_i32 rstomp = {ohero->pos.x - HERO_W_STOMP_ADD_SYMM,
                          ohero->pos.y + ohero->h,
                          ohero->w + HERO_W_STOMP_ADD_SYMM * 2,
                          1};

        if (overlap_rec(rstomp, rplat)) {
            obj_step_actor(g, ohero, +0, -1);

            if ((o->flags & OBJ_FLAG_HERO_STOMPABLE) && hero_stomping(ohero)) {
                hero_register_jumpstomped(ohero, o, 1);
            }
            if ((o->flags & OBJ_FLAG_HERO_JUMPABLE)) {
                hero_register_jumpstomped(ohero, o, 0);
            }
        }
    }

    o->pos.x += sx;
    o->pos.y += sy;
    if (o->ropenode) {
        ropenode_move(g, o->rope, o->ropenode, sx, sy);
    }
}
