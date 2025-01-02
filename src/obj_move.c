// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "obj.h"
#include "rope.h"

bool32 obj_blocked_by_map_or_objs(g_s *g, obj_s *o, i32 sx, i32 sy)
{
    if (!(o->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS)) return 0;

    rec_i32 ro = {o->pos.x + sx, o->pos.y + sy, o->w, o->h};
    if (tile_map_solid(g, ro)) return 1;
    for (obj_each(g, i)) {
        if (i == o || i->mass == 0 || i->mass < o->mass) continue;
        rec_i32 ri = obj_aabb(i);
        if (overlap_rec(ro, ri)) {
            return 1;
        }
    }
    return 0;
}

bool32 obj_on_platform(g_s *g, obj_s *o, i32 x, i32 y, i32 w)
{
    if (!(o->moverflags & OBJ_MOVER_ONE_WAY_PLAT)) return 0;
    if (!(o->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS)) return 0;

    rec_i32 r     = {x, y, w, 1};
    rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
    rec_i32 ri;
    if ((y & 15) == 0 && intersect_rec(r, rgrid, &ri)) {
        i32 ty  = (ri.y) >> 4;
        i32 tx0 = (ri.x) >> 4;
        i32 tx1 = (ri.x + ri.w - 1) >> 4;

        for (i32 tx = tx0; tx <= tx1; tx++) {
            switch (g->tiles[tx + ty * g->tiles_x].collision) {
            case TILE_ONE_WAY:
            case TILE_LADDER_ONE_WAY:
                return 1;
            }
        }
    }

    for (obj_each(g, it)) {
        if (it == o) continue;
        if (!(it->flags & OBJ_FLAG_PLATFORM)) continue;
        rec_i32 rplat = {it->pos.x, it->pos.y, it->w, 1};
        if (!overlap_rec(r, rplat)) continue;
        return 1;
    }
    return 0;
}

bool32 obj_blocked_by_platform(g_s *g, obj_s *o, i32 x, i32 y, i32 w)
{
    if (!(o->moverflags & OBJ_MOVER_ONE_WAY_PLAT)) return 0;
    if (!(o->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS)) return 0;

    rec_i32 r     = {x, y, w, 1};
    rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
    rec_i32 ri;
    if ((y & 15) == 0 && intersect_rec(r, rgrid, &ri)) {
        i32 ty  = (ri.y) >> 4;
        i32 tx0 = (ri.x) >> 4;
        i32 tx1 = (ri.x + ri.w - 1) >> 4;

        for (i32 tx = tx0; tx <= tx1; tx++) {
            switch (g->tiles[tx + ty * g->tiles_x].collision) {
            case TILE_ONE_WAY:
            case TILE_LADDER_ONE_WAY:
                return 1;
            }
        }
    }

    bool32 is_plat = 0;

    for (obj_each(g, it)) {
        if (it == o) continue;

        rec_i32 rplat = {it->pos.x, it->pos.y, it->w, 1};
        if (overlap_rec(r, rplat)) {
            is_plat |= it->flags & OBJ_FLAG_PLATFORM;
            is_plat |= (o->ID == OBJ_ID_HERO) &&
                       (it->flags & OBJ_FLAG_HERO_PLATFORM);
        }
    }

    if (o->ID == OBJ_ID_HERO) {
        rec_i32 rstomp = r;
        rstomp.x -= HERO_W_STOMP_ADD_SYMM;
        rstomp.w += HERO_W_STOMP_ADD_SYMM * 2;

        for (obj_each(g, it)) {
            if (it == o) continue;

            rec_i32 rplat = {it->pos.x, it->pos.y, it->w, 1};
            if (overlap_rec(rstomp, rplat)) {
                if (hero_stomping(o) && (it->flags & OBJ_FLAG_HERO_STOMPABLE)) {
                    is_plat |= 1;
                    hero_register_stomped_on(o, it);
                }
                if ((it->flags & OBJ_FLAG_HERO_JUMPABLE)) {
                    is_plat |= 1;
                    hero_register_jumped_on(o, it);
                }
            }
        }
    }
    return is_plat;
}

void obj_move(g_s *g, obj_s *o, i32 dx, i32 dy)
{
    i32 px = +abs_i32(dx);
    i32 py = -abs_i32(dy);
    i32 sx = +sgn_i32(dx);
    i32 sy = +sgn_i32(dy);
    i32 e  = px + py;
    i32 x  = 0;
    i32 y  = 0;

    while (x != dx || y != dy) {
        i32 e2 = e << 1;
        if (e2 >= py) {
            e += py;
            x += sx;

            b32 was_grounded = obj_grounded(g, o);
            b32 stepped_x    = obj_step(g, o, sx, 0, 1, 0);

            // glue ground conditions
            if ((o->moverflags & OBJ_MOVER_GLUE_GROUND) &&
                stepped_x &&
                was_grounded &&
                !obj_grounded(g, o) &&
                !obj_blocked_by_map_or_objs(g, o, 0, 1) &&
                obj_grounded_at_offs(g, o, (v2_i32){0, 1})) {
                obj_step(g, o, 0, +1, 0, 0);
            }
        }
        if (e2 <= px) {
            e += px;
            y += sy;
            b32 stepped_y = obj_step(g, o, 0, sy, 1, 0);
        }
    }
}

b32 obj_step(g_s *g, obj_s *o, i32 sx, i32 sy, b32 can_slide, i32 m_push)
{
    b32 slide_l = sy && can_slide && (o->moverflags & OBJ_MOVER_SLIDE_X_NEG);
    b32 slide_r = sy && can_slide && (o->moverflags & OBJ_MOVER_SLIDE_X_POS);
    b32 slide_u = sx && can_slide && (o->moverflags & OBJ_MOVER_SLIDE_Y_NEG);
    b32 slide_d = sx && can_slide && (o->moverflags & OBJ_MOVER_SLIDE_Y_POS);

    i32 m_og = o->mass;
    if (m_push && o->mass) { // assume the pusher's mass until moving is done
        o->mass = m_push;
    }

    b32 blocked = 0;
    if (o->flags & OBJ_FLAG_CLAMP_ROOM_X) {
        blocked |= (o->pos.x + sx < 0);
        blocked |= (g->pixel_x < o->pos.x + sx + o->w);
    }
    if (o->flags & OBJ_FLAG_CLAMP_ROOM_Y) {
        blocked |= (o->pos.y + sy < 0);
        blocked |= (g->pixel_y < o->pos.y + sy + o->h);
    }

    if (!blocked && obj_blocked_by_map_or_objs(g, o, sx, sy)) {
        if (0) {
        } else if (slide_u && !obj_blocked_by_map_or_objs(g, o, sx, -1)) {
            obj_step(g, o, +0, -1, 0, o->mass);
        } else if (slide_d && !obj_blocked_by_map_or_objs(g, o, sx, +1)) {
            obj_step(g, o, +0, +1, 0, o->mass);
        } else if (slide_l && !obj_blocked_by_map_or_objs(g, o, -1, sy)) {
            obj_step(g, o, -1, +0, 0, o->mass);
        } else if (slide_r && !obj_blocked_by_map_or_objs(g, o, +1, sy)) {
            obj_step(g, o, +1, +0, 0, o->mass);
        } else if ((o->moverflags & OBJ_MOVER_AVOID_HEADBUMP) && sy == -1) {
            blocked = 1;
        } else {
            blocked = 1;
        }
    }

    b32 blocked_oneway = 0 < sy &&
                         obj_blocked_by_platform(g, o, o->pos.x, o->pos.y + o->h, o->w);

    blocked |= (blocked_oneway && !m_push);
    if (blocked) {
        o->bumpflags |= obj_bump_x_flag(sx);
        o->bumpflags |= obj_bump_y_flag(sy);
    } else {
        rec_i32 cr = obj_aabb(o);
        if (o->mass && g->rope.active) {
            rope_moved_by_aabb(g, &g->rope, cr, sx, sy);
        }

        o->pos.x += sx;
        o->pos.y += sy;

        if (o->ropenode) {
            ropenode_move(g, o->rope, o->ropenode, sx, sy);
        }

        rec_i32 oaabb = obj_aabb(o);
        rec_i32 rplat = {o->pos.x, o->pos.y - sy, o->w, 1};
        obj_s  *ohero = obj_get_hero(g);
        if (ohero && ohero != o &&
            (o->flags & (OBJ_FLAG_HERO_JUMPABLE | OBJ_FLAG_HERO_STOMPABLE))) {

            rec_i32 rstomp = obj_rec_bottom(ohero);
            rstomp.x -= HERO_W_STOMP_ADD_SYMM;
            rstomp.w += HERO_W_STOMP_ADD_SYMM * 2;
            if (overlap_rec(rstomp, rplat)) {
                obj_step(g, ohero, +0, sy, 1, 0);
                if (hero_stomping(ohero) && (o->flags & OBJ_FLAG_HERO_STOMPABLE)) {
                    hero_register_stomped_on(ohero, o);
                }
                if ((o->flags & OBJ_FLAG_HERO_JUMPABLE)) {
                    hero_register_jumped_on(ohero, o);
                }
            }
        }

        if (o->mass || (o->flags & OBJ_FLAG_PLATFORM)) { // solid pushing
            for (obj_each(g, i)) {
                if (o == i) continue;
                if (!(i->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS)) continue;

                rec_i32 it_bot = obj_rec_bottom(i);
                rec_i32 it_rec = obj_aabb(i);
                b32     linked = o == obj_from_obj_handle(i->linked_solid);
                b32     pushed = 0;

                if (o->mass) {
                    pushed |= i->mass < o->mass && overlap_rec(oaabb, it_rec);
                    linked |= i->mass <= o->mass && overlap_rec(cr, it_bot);
                }

                if (i->moverflags & OBJ_MOVER_ONE_WAY_PLAT) {
                    b32 is_plat = 0;
                    is_plat |= o->flags & OBJ_FLAG_PLATFORM;
                    is_plat |= (i->ID == OBJ_ID_HERO) &&
                               (o->flags & OBJ_FLAG_HERO_PLATFORM);
                    linked |= (is_plat && overlap_rec(it_bot, rplat));
                }

                if (linked | pushed) {
                    obj_step(g, i, sx, +0, 1, pushed ? o->mass : 0);
                    obj_step(g, i, +0, sy, 1, pushed ? o->mass : 0);
                }
            }
        }
    }
    o->mass = m_og;
    return (!blocked);
}