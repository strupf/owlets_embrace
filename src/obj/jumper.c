// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    JUMPER_ST_IDLE,
    JUMPER_ST_HURT,
    JUMPER_ST_DIE,
    JUMPER_ST_ANTICIPATE,
    JUMPER_ST_JUMPING,
    JUMPER_ST_LANDED,
};

#define JUMPER_TICKS_ANTICIPATE 15
#define JUMPER_TICKS_LAND       8

void jumper_on_update(g_s *g, obj_s *o);
void jumper_on_animate(g_s *g, obj_s *o);
void jumper_on_hurt(g_s *g, obj_s *o);

void jumper_load(g_s *g, map_obj_s *mo)
{
    obj_s *o      = obj_create(g);
    o->UUID       = mo->UUID;
    o->ID         = OBJID_JUMPER;
    o->on_update  = jumper_on_update;
    o->on_animate = jumper_on_animate;
    o->w          = 24;
    o->h          = 18;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y + mo->h - o->h;
    o->facing     = +1;
    if (map_obj_bool(mo, "face_left")) {
        o->facing = -1;
    }

    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_KILL_OFFSCREEN |
               // OBJ_FLAG_HERO_JUMPSTOMPABLE |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_CLAMP_ROOM_X |
               0;
    o->moverflags = OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_TERRAIN_COLLISIONS;
    o->health_max = 2;
    o->health     = o->health_max;
}

void jumper_on_update(g_s *g, obj_s *o)
{
    o->animation++;

    if (o->bumpflags & OBJ_BUMP_X) {
        o->v_q12.x = -o->v_q12.x / 2;
    }
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q12.y = 0;
    }
    o->bumpflags = 0;

    o->v_q12.y += Q_VOBJ(0.35);
    bool32 grounded = obj_grounded(g, o);
    if (!grounded && o->state != JUMPER_ST_JUMPING) {
        o->state = JUMPER_ST_JUMPING;
        o->timer = 0;
    }

    if (grounded) {
        o->v_q12.x = 0;
    }

    switch (o->state) {
    case JUMPER_ST_HURT: {
        o->timer++;
        if (ENEMY_HIT_FREEZE_TICKS <= o->timer) {
            o->timer = 0;
            if (grounded) {
                o->state = JUMPER_ST_IDLE;
            } else {
                o->state = JUMPER_ST_JUMPING;
            }
        }
        break;
    }
    case JUMPER_ST_IDLE: {
        o->timer++;
        obj_s *ohero = obj_get_owl(g);
        if (ohero) {
            v2_i32 phero = obj_pos_center(ohero);
            v2_i32 pc    = obj_pos_center(o);
            if (v2_i32_distancesq(phero, pc) < 14000) {
                o->state     = JUMPER_ST_ANTICIPATE;
                o->timer     = 0;
                o->animation = 0;
                o->facing    = phero.x < pc.x ? -1 : +1;
            }
        }
        break;
    }
    case JUMPER_ST_ANTICIPATE: {
        o->timer++;
        if (JUMPER_TICKS_ANTICIPATE <= o->timer) {
            o->state     = JUMPER_ST_JUMPING;
            o->timer     = 0;
            o->v_q12.y   = -Q_VOBJ(8.0);
            o->v_q12.x   = o->facing * Q_VOBJ(2.2) + rngr_sym_i32(Q_VOBJ(0.7));
            o->animation = 0;
            snd_play(SNDID_SPEAR_ATTACK, 0.75f, 1.f);
        }
        break;
    }
    case JUMPER_ST_JUMPING: {
        o->timer++;
        if (grounded) {
            snd_play(SNDID_STOMP_LAND, 1.1f, 1.f);
            o->state     = JUMPER_ST_LANDED;
            o->timer     = 0;
            o->animation = 0;

            v2_i32 pp = obj_pos_bottom_center(o);
            v2_i32 pr = {pp.x + 8, pp.y};
            v2_i32 pl = {pp.x - 8, pp.y};

            objanim_create(g, pl, OBJANIMID_STOMP_L);
            objanim_create(g, pr, OBJANIMID_STOMP_R);
        }
        break;
    }
    case JUMPER_ST_LANDED: {
        o->timer++;
        if (JUMPER_TICKS_LAND <= o->timer) {
            o->timer     = 0;
            o->state     = JUMPER_ST_IDLE;
            o->animation = 0;
        }
        break;
    }
    }

    obj_move_by_v_q12(g, o);
}

void jumper_on_hurt(g_s *g, obj_s *o)
{
    if (o->state == JUMPER_ST_DIE) return;

    o->v_q12.x = 0;
    o->v_q12.y = 0;
    if (o->health) {
        o->state = JUMPER_ST_HURT;
    } else {
        o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
        o->state     = JUMPER_ST_DIE;
        o->on_update = enemy_on_update_die;
        g->enemies_killed++;
        g->enemy_killed[ENEMYID_JUMPER]++;
    }
    o->timer = 0;
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

    switch (o->state) {
    case JUMPER_ST_DIE:
    case JUMPER_ST_HURT: {
        fr_y = 0;
        fr_x = (o->timer < ENEMY_HIT_FLASH_TICKS ? 4 : 5);
        spr->offs.x += rngr_sym_i32(ENEMY_HIT_FREEZE_SHAKE_AMOUNT);
        spr->offs.y += rngr_sym_i32(ENEMY_HIT_FREEZE_SHAKE_AMOUNT);
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
        } else if (o->v_q12.y < -Q_VOBJ(0.8)) {
            fr_x = 2;
        } else if (o->v_q12.y < +Q_VOBJ(1.2)) {
            fr_x = 3;
        } else if (o->v_q12.y < +Q_VOBJ(2.3)) {
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
