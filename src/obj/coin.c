// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define COIN_TIME          500
#define COIN_HOMING_DISTSQ POW2(40)

enum {
    COIN_ST_IDLE,
    COIN_ST_PREHOMING,
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
    o->timer           = COIN_TIME;
    o->animation       = rngr_i32(0, 50);
    o->n_sprites       = 1;
    obj_sprite_s *spr  = &o->sprites[0];
    spr->flip          = gfx_spr_flip_rng(1, 0);
    o->render_priority = RENDER_PRIO_OWL + 1;
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
    i32 fry     = ani_frame_loop(ANIID_GEMS, o->animation);
    i32 frx     = o->subID;
    spr->offs.x = (o->w - 32) / 2;
    spr->offs.y = (o->h - 24) / 2;
    spr->trec   = asset_texrec(TEXID_GEMS, frx * 32, fry * 24, 32, 24);
}

void coin_on_update(g_s *g, obj_s *o)
{
    obj_s *ohero = obj_get_owl(g);
    v2_i32 phero = {0};
    if (ohero) {
        phero = obj_pos_center(ohero);
    }
    v2_i32 p = obj_pos_center(o);
    o->subtimer++;

    switch (o->state) {
    case COIN_ST_IDLE:
    case COIN_ST_PREHOMING: {
        o->v_q12.y += Q_VOBJ(0.27);
        o->v_q12.y = min_i32(o->v_q12.y, Q_VOBJ(6.0));

        if (obj_grounded(g, o)) {
            obj_vx_q8_mul(o, 220);
        }

        if (o->bumpflags & OBJ_BUMP_X) {
            obj_vx_q8_mul(o, -192);
        }
        if (o->bumpflags & OBJ_BUMP_Y) {
            if (o->v_q12.y < Q_VOBJ(0.75)) {
                o->v_q12.y = 0;
            } else {
                obj_vx_q8_mul(o, +210);
                obj_vy_q8_mul(o, -160);
            }
        }
        o->bumpflags = 0;

        if (abs_i32(o->v_q12.x) < Q_VOBJ(0.1)) {
            o->v_q12.x = 0;
        }

        // already tagged for collect, but keep coins for a minimum time on screen
        if (o->state == COIN_ST_PREHOMING) {
            if (10 <= o->subtimer) {
                o->state      = COIN_ST_HOMING;
                o->moverflags = 0;
            }
        } else if (ohero && v2_i32_distancesq(phero, p) <= COIN_HOMING_DISTSQ) { // hero attract coins
            o->state    = COIN_ST_PREHOMING;
            o->timer    = 0;
            o->blinking = 0;
        } else if (--o->timer <= 0) {
            obj_delete(g, o);
        } else if (o->timer < 120) {
            o->blinking = 1;
        }
        break;
    }
    case COIN_ST_HOMING: {
        if (ohero) {
            v2_i32 vs = steer_seek(p, o->v_q12, phero, Q_VOBJ(4.7));
            o->v_q12.x += vs.x >> 1;
            o->v_q12.y += vs.y >> 1;
        } else { // consider it collected
            coins_change(g, +1);
            obj_delete(g, o);
        }
        break;
    }
    }

    obj_move_by_v_q12(g, o);
}

void coin_try_collect(g_s *g, obj_s *o, v2_i32 heropos)
{
    if (o->state == COIN_ST_HOMING && v2_i32_distancesq(heropos, obj_pos_center(o)) < POW2(50)) {
        coins_change(g, +1);
        snd_play(SNDID_COIN, 0.5f, rngr_f32(0.95f, 1.05f));
        obj_delete(g, o);
    }
}