// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define COIN_TIME          500
#define COIN_HOMING_DISTSQ POW2(40)

enum {
    COIN_ST_IDLE,
    COIN_ST_HOMING,
};

void coin_on_animate(g_s *g, obj_s *o);
void coin_on_update(g_s *g, obj_s *o);

obj_s *coin_create(g_s *g)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJID_COIN;
    o->subID      = rngr_i32(0, 1);
    o->on_update  = coin_on_update;
    o->on_animate = coin_on_animate;
    o->w          = 8;
    o->h          = 8;
    o->flags      = OBJ_FLAG_ACTOR;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS |
                    OBJ_MOVER_ONE_WAY_PLAT;
    o->timer          = COIN_TIME;
    o->animation      = rngr_i32(0, 50);
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->flip         = gfx_spr_flip_rng(1, 0);
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
    obj_sprite_s *spr = &o->sprites[0];
    o->animation++;
    i32 fry     = ani_frame(ANIID_GEMS, o->animation);
    i32 frx     = o->subID;
    spr->offs.x = (o->w - 32) / 2;
    spr->offs.y = (o->h - 24) / 2;
    spr->trec   = asset_texrec(TEXID_GEMS, frx * 32, fry * 24, 32, 24);
}

void coin_on_update(g_s *g, obj_s *o)
{
    obj_s *ohero = obj_get_hero(g);
    v2_i32 phero = {0};
    if (ohero) {
        phero = obj_pos_center(ohero);
    }
    v2_i32 p = obj_pos_center(o);

    switch (o->state) {
    case COIN_ST_IDLE: {
        // hero attract coins
        if (ohero && v2_i32_distancesq(phero, p) <= COIN_HOMING_DISTSQ) {
            o->state      = COIN_ST_HOMING;
            o->moverflags = 0;
            break;
        }

        if (--o->timer <= 0) {
            obj_delete(g, o);
            break;
        }

        if (o->timer < 120) {
            o->blinking = 1;
        }

        o->v_q8.y += 70;
        o->v_q8.y = min_i32(o->v_q8.y, 256 * 6);

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
                obj_vx_q8_mul(o, +210);
                obj_vy_q8_mul(o, -160);
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
        if (ohero) {
            v2_i32 v  = v2_i32_from_i16(o->v_q8);
            v2_i32 vs = steer_seek(p, v, phero, 900);
            o->v_q8.x += vs.x >> 1;
            o->v_q8.y += vs.y >> 1;
        } else {
            coins_change(g, +1);
            obj_delete(g, o);
        }

        break;
    }
    }

    obj_move_by_v_q8(g, o);
}