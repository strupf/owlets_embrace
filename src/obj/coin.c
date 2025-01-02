// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define COIN_TIME          2000
#define COIN_HOMING_DISTSQ POW2(50)

enum {
    COIN_ST_IDLE,
    COIN_ST_HOMING,
};

obj_s *coin_create(g_s *g)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJ_ID_COIN;
    o->w          = 16;
    o->h          = 16;
    o->flags      = OBJ_FLAG_RENDER_AABB;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS | OBJ_MOVER_ONE_WAY_PLAT;
    o->timer      = COIN_TIME;
    return o;
}

void coin_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = coin_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y;
}

void coin_on_animate(g_s *g, obj_s *o)
{
}

void coin_on_update(g_s *g, obj_s *o)
{
    switch (o->state) {
    case COIN_ST_IDLE: {
        if (hero_has_charm(g, HERO_CHARM_ATTRACT_COINS)) {
            obj_s *ohero = obj_get_hero(g);
            if (ohero) {
                v2_i32 phero = obj_pos_center(ohero);
                v2_i32 p     = obj_pos_center(o);
                if (v2_distancesq(phero, p) <= COIN_HOMING_DISTSQ) {
                    o->state      = 1;
                    o->moverflags = 0;
                    break;
                }
            }
        }

        if (--o->timer <= 0) {
            obj_delete(g, o);
            break;
        }

        o->v_q8.y += 60;
        obj_move_by_v_q8(g, o);

        if (obj_grounded(g, o)) {
            obj_vx_q8_mul(o, 230);
        }

        if (o->bumpflags & OBJ_BUMP_X) {
            obj_vx_q8_mul(o, -192);
        }
        if (o->bumpflags & OBJ_BUMP_Y) {
            if (o->v_q8.y < 400) {
                o->v_q8.y = 0;
            } else {
                obj_vy_q8_mul(o, -192);
            }
        }
        o->bumpflags = 0;

        if (abs_i32(o->v_q8.x) < 16) {
            o->v_q8.x = 0;
        }
        break;
    }
    case COIN_ST_HOMING: {
        obj_s *ohero = obj_get_hero(g);
        if (!ohero) {
            o->state      = COIN_ST_IDLE;
            o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS | OBJ_MOVER_ONE_WAY_PLAT;
            break;
        }

        break;
    }
    }
}