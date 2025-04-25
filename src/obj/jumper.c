// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    JUMPER_ST_HURT = -1,
    JUMPER_ST_IDLE,
    JUMPER_ST_ANTICIPATE,
    JUMPER_ST_JUMPING,
    JUMPER_ST_LANDED,
};

#define JUMPER_TICKS_ANTICIPATE 20
#define JUMPER_TICKS_LAND       8

void jumper_on_update(g_s *g, obj_s *o);
void jumper_on_animate(g_s *g, obj_s *o);
void jumper_on_hurt(g_s *g, obj_s *o);

void jumper_load(g_s *g, map_obj_s *mo)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJID_JUMPER;
    o->on_update  = jumper_on_update;
    o->on_animate = jumper_on_animate;
    o->w          = 16;
    o->h          = 16;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->facing     = 1;

    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_HERO_JUMPSTOMPABLE |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_CLAMP_ROOM_X |
               0;
    o->moverflags = OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_TERRAIN_COLLISIONS;
    o->health_max         = 2;
    o->health             = o->health_max;
    o->enemy              = enemy_default();
    o->enemy.on_hurt      = jumper_on_hurt;
    o->enemy.hurt_on_jump = 1;
}

void jumper_on_update(g_s *g, obj_s *o)
{
    o->timer++;
    o->animation++;

    if (o->bumpflags & OBJ_BUMP_X) {
        o->v_q8.x = -o->v_q8.x / 2;
    }
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q8.y = 0;
    }
    o->bumpflags = 0;

    o->v_q8.y += 90;
    bool32 grounded = obj_grounded(g, o);
    if (!grounded && o->state != JUMPER_ST_JUMPING) {
        o->state = JUMPER_ST_JUMPING;
        o->timer = 0;
    }

    if (grounded) {
        o->v_q8.x = 0;
    }

    switch (o->state) {
    case JUMPER_ST_IDLE: {
        obj_s *ohero = obj_get_hero(g);
        if (ohero) {
            v2_i32 phero = obj_pos_center(ohero);
            v2_i32 pc    = obj_pos_center(o);
            if (v2_i32_distancesq(phero, pc) < 12000) {
                o->state     = JUMPER_ST_ANTICIPATE;
                o->timer     = 0;
                o->animation = 0;
                o->facing    = phero.x < pc.x ? -1 : +1;
            }
        }
        break;
    }
    case JUMPER_ST_ANTICIPATE: {
        if (JUMPER_TICKS_ANTICIPATE <= o->timer) {
            o->state     = JUMPER_ST_JUMPING;
            o->timer     = 0;
            o->v_q8.y    = -2000;
            o->v_q8.x    = o->facing * 500 + rngr_sym_i32(300);
            o->animation = 0;
        }
        break;
    }
    case JUMPER_ST_JUMPING: {
        if (grounded) {
            o->state     = JUMPER_ST_LANDED;
            o->timer     = 0;
            o->animation = 0;
        }
        break;
    }
    case JUMPER_ST_LANDED: {
        if (JUMPER_TICKS_LAND <= o->timer) {
            o->timer     = 0;
            o->state     = JUMPER_ST_IDLE;
            o->animation = 0;
        }
        break;
    }
    }

    obj_move_by_v_q8(g, o);
}

void jumper_on_hurt(g_s *g, obj_s *o)
{
    o->v_q8.x = 0;
    o->v_q8.y = 0;
    o->timer  = 0;
}

void jumper_on_animate(g_s *g, obj_s *o)
{
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->offs.x       = (o->w - 64) / 2;
    spr->offs.y       = (o->h - 64) + 2;
    spr->flip         = 0 < o->facing ? SPR_FLIP_X : 0;

    i32 fr_x = 0;
    i32 fr_y = 0;
    i32 st   = o->enemy.hurt_tick ? JUMPER_ST_HURT : o->state;

    switch (st) {
    case JUMPER_ST_HURT: {
        fr_y = 0;
        fr_x = 5;
        break;
    }
    case JUMPER_ST_IDLE: {
        fr_x = ((o->animation >> 3) & 3);
        break;
    }
    case JUMPER_ST_ANTICIPATE: {
        fr_y = 1;
        fr_x = (o->animation >> 2) & 1;
        break;
    }
    case JUMPER_ST_JUMPING: {
        fr_y = 2;
        spr->offs.y += 10;
        if (o->timer < 6) {
            fr_x = (3 <= o->timer);
        } else if (o->v_q8.y < -200) {
            fr_x = 2;
        } else if (o->v_q8.y < +300) {
            fr_x = 3;
        } else if (o->v_q8.y < +800) {
            fr_x = 4;
        } else {
            fr_x = 5;
        }
        break;
    }
    case JUMPER_ST_LANDED: {
        fr_y = 1;
        fr_x = o->timer < 4 ? 1 : 0;
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_JUMPER, fr_x * 64, fr_y * 64, 64, 64);
}
