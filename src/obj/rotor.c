// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    ROTOR_IDLE,
    ROTOR_ALERT,
    ROTOR_ATTACK,
    ROTOR_SHOW_UP,
};

#define ROTOR_IDLE_TICKS_MIN  50
#define ROTOR_ALERT_TICKS     20
#define ROTOR_ATTACK_TICKS    150
#define ROTOR_SHOW_UP_TICKS   30
#define ROTOR_V_MAX           1024
#define ROTOR_DETECT_BOX_W    92
#define ROTOR_DETECT_BOX_H    48
#define ROTOR_ROT_SPEED_TICKS 35
#define ROTOR_ROT_SPEED_MAX   192
#define ROTOR_ROT_T_SLOWDOWN  (ROTOR_ATTACK_TICKS - ROTOR_ROT_SPEED_TICKS)

typedef struct {
    i32 frame;
    i32 rot_speed;
    i32 rot_dir;
} rotor_s;

void rotor_load(g_s *g, map_obj_s *mo)
{
    obj_s   *o = obj_create(g);
    rotor_s *r = (rotor_s *)o->mem;

    o->ID         = OBJ_ID_ROTOR;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->w          = 26;
    o->h          = 18;
    o->n_sprites  = 1;
    r->rot_speed  = 64;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS |
                    OBJ_MOVER_ONE_WAY_PLAT;
    o->health = 2;
    o->flags  = OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_HERO_JUMPABLE |
               OBJ_FLAG_HERO_STOMPABLE;
}

void rotor_on_update(g_s *g, obj_s *o)
{
    rotor_s *r = (rotor_s *)o->mem;
    o->flags |= (OBJ_FLAG_HERO_STOMPABLE | OBJ_FLAG_HERO_JUMPABLE);
    o->timer++;
    o->v_q8.y += 60;

    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q8.y = 0;
    }
    if (o->bumpflags & OBJ_BUMP_X) {
        o->v_q8.x = -o->v_q8.x;
    }
    o->bumpflags = 0;

    bool32 grounded = obj_grounded(g, o);

    switch (o->state) {
    case ROTOR_IDLE:
    case ROTOR_ALERT:
    case ROTOR_SHOW_UP:
        if (grounded) {
            o->v_q8.x = 0;
        }
        break;
    }

    switch (o->state) {
    case ROTOR_IDLE: {
        if (o->timer < ROTOR_IDLE_TICKS_MIN) break;
        obj_s *ohero       = obj_get_hero(g);
        i32    notice_hero = 0;

        if (ohero) {
            rec_i32 hero_aabb = obj_aabb(ohero);
            rec_i32 trigger_r = {o->pos.x + o->w,
                                 o->pos.y + o->h - ROTOR_DETECT_BOX_H,
                                 ROTOR_DETECT_BOX_W,
                                 ROTOR_DETECT_BOX_H};
            rec_i32 trigger_l = {o->pos.x - ROTOR_DETECT_BOX_W,
                                 o->pos.y + o->h - ROTOR_DETECT_BOX_H,
                                 ROTOR_DETECT_BOX_W,
                                 ROTOR_DETECT_BOX_H};

            if (overlap_rec(hero_aabb, trigger_r)) {
                notice_hero = +1;
            }
            if (overlap_rec(hero_aabb, trigger_l)) {
                notice_hero = -1;
            }
        }

        if (notice_hero) {
            o->timer   = 0;
            o->state   = ROTOR_ALERT;
            r->rot_dir = notice_hero;
        }
        break;
    }
    case ROTOR_ALERT: {
        if (ROTOR_ALERT_TICKS <= o->timer) {
            o->timer     = 0;
            o->state     = ROTOR_ATTACK;
            o->animation = 0;
            o->v_q8.x    = r->rot_dir * 256;
        }
        break;
    }
    case ROTOR_ATTACK: {
        o->flags &= ~(OBJ_FLAG_HERO_STOMPABLE | OBJ_FLAG_HERO_JUMPABLE);

        if (!grounded) {
            o->timer = ROTOR_ATTACK_TICKS / 2;
        }

        i32 t_slowdown = ROTOR_ATTACK_TICKS - ROTOR_ROT_SPEED_TICKS;
        if (o->timer < ROTOR_ROT_SPEED_TICKS) {
            i32 num      = o->timer;
            i32 den      = ROTOR_ROT_SPEED_TICKS;
            r->rot_speed = lerp_i32(0, ROTOR_ROT_SPEED_MAX, num, den);
            o->v_q8.x    = sgn_i32(o->v_q8.x) * ease_out_quad(1, ROTOR_V_MAX, num, den);
        } else if (ROTOR_ROT_T_SLOWDOWN <= o->timer) {
            i32 num      = o->timer - ROTOR_ROT_T_SLOWDOWN;
            i32 den      = ROTOR_ROT_SPEED_TICKS;
            r->rot_speed = lerp_i32(ROTOR_ROT_SPEED_MAX, 0, num, den);
            o->v_q8.x    = sgn_i32(o->v_q8.x) * ease_out_quad(ROTOR_V_MAX, 1, num, den);
        } else {
            r->rot_speed = ROTOR_ROT_SPEED_MAX;
            o->v_q8.x    = sgn_i32(o->v_q8.x) * ROTOR_V_MAX;
        }

        if (ROTOR_ATTACK_TICKS <= o->timer && grounded) {
            o->timer = 0;
            o->state = ROTOR_SHOW_UP;
        }
        break;
    }
    case ROTOR_SHOW_UP: {
        if (ROTOR_SHOW_UP_TICKS <= o->timer) {
            o->timer = 0;
            o->state = ROTOR_IDLE;
        }
        break;
    }
    }

    obj_move_by_v_q8(g, o);
}

void rotor_on_animate(g_s *g, obj_s *o)
{
    rotor_s      *r   = (rotor_s *)o->mem;
    obj_sprite_s *spr = &o->sprites[0];

    i32 state   = o->state;
    i32 fr_x    = 0;
    i32 fr_y    = 0;
    spr->offs.x = (o->w - 48) / 2;
    spr->offs.y = -(48 - o->h);

    if (o->enemy.hurt_tick) { // display hurt frame
        fr_x  = 0;
        fr_y  = 3;
        state = -1;
    }

    switch (state) {
    case ROTOR_IDLE: {
        o->animation++;
        fr_x = (o->animation >> 3) & 3;
        break;
    }
    case ROTOR_ALERT: {
        fr_y = 1;
        fr_x = min_i32(lerp_i32(0, 4, o->timer, ROTOR_ALERT_TICKS), 3);
        break;
    }
    case ROTOR_ATTACK: {
        o->animation += r->rot_speed;
        while (256 <= o->animation) {
            r->frame = (r->frame + 1) & 3;
            o->animation -= 256;
        }
        fr_x = r->frame;
        fr_y = 2;
        break;
    }
    case ROTOR_SHOW_UP: {
        fr_y = 1;
        fr_x = 3 - min_i32(lerp_i32(0, 4, o->timer, ROTOR_SHOW_UP_TICKS), 3);
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_ROTOR, fr_x * 48, fr_y * 48, 48, 48);
}
