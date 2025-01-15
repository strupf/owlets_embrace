// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "obj.h"
#include "rope.h"

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
        if (overlap_rec(r, rplat))
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
            is_plat |= (o->ID == OBJID_HERO) &&
                       (it->flags & OBJ_FLAG_HERO_PLATFORM);
        }
    }

    if (o->ID == OBJID_HERO) {
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

void obj_move_actor(g_s *g, obj_s *o, i32 dx, i32 dy);
void obj_move_solid(g_s *g, obj_s *o, i32 dx, i32 dy);

void obj_move(g_s *g, obj_s *o, i32 dx, i32 dy)
{
    if (o->flags & OBJ_FLAG_SOLID) {
        obj_move_solid(g, o, dx, dy);
    } else {
        obj_move_actor(g, o, dx, dy);
    }
}

bool32 obj_step_is_clamped(g_s *g, obj_s *o, i32 sx, i32 sy)
{
    if (o->flags & OBJ_FLAG_CLAMP_ROOM_X) {
        if (sx < 0 && o->pos.x == 0) {
            o->bumpflags |= OBJ_BUMP_X_NEG;
            return 1;
        }
        if (sx > 0 && o->pos.x + o->w == g->pixel_x) {
            o->bumpflags |= OBJ_BUMP_X_POS;
            return 1;
        }
    }
    if (o->flags & OBJ_FLAG_CLAMP_ROOM_Y) {
        if (sy < 0 && o->pos.y == 0) {
            o->bumpflags |= OBJ_BUMP_Y_NEG;
            return 1;
        }
        if (sy > 0 && o->pos.y + o->h == g->pixel_y) {
            o->bumpflags |= OBJ_BUMP_Y_POS;
            return 1;
        }
    }
    return 0;
}