// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    BUDPLANT_ST_IDLE,
    BUDPLANT_ST_PREPARING,
    BUDPLANT_ST_SHAKING,
    BUDPLANT_ST_SHOOTING
};

typedef struct {
    i32 x;
} budplant_s;

#define BUDPLANT_TICKS_PREPARE  25
#define BUDPLANT_TICKS_SHAKING  30
#define BUDPLANT_TICKS_SHOOTING 25

void bpgrenade_create(g_s *g, v2_i32 pos, v2_i32 vel);
//
void budplant_on_hurt(g_s *g, obj_s *o);
void budplant_on_update(g_s *g, obj_s *o);
void budplant_on_animate(g_s *g, obj_s *o);

void budplant_load(g_s *g, map_obj_s *mo)
{
    obj_s      *o  = obj_create(g);
    budplant_s *bp = (budplant_s *)o->mem;

    o->ID    = OBJID_BUDPLANT;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_HERO_JUMPSTOMPABLE;
    o->moverflags         = OBJ_MOVER_TERRAIN_COLLISIONS;
    o->on_update          = budplant_on_update;
    o->on_animate         = budplant_on_animate;
    o->w                  = 24;
    o->h                  = 24;
    o->pos.x              = mo->x + (mo->w - o->w) / 2;
    o->pos.y              = mo->y + (mo->h - o->h);
    o->health_max         = 2;
    o->health             = o->health_max;
    o->enemy              = enemy_default();
    o->enemy.hurt_on_jump = 1;
    o->enemy.on_hurt      = budplant_on_hurt;
    o->timer              = rngr_i32(0, 100);

    obj_sprite_s *spr = &o->sprites[0];
    o->n_sprites      = 1;
}

void budplant_on_update(g_s *g, obj_s *o)
{
    budplant_s *bp = (budplant_s *)o->mem;
    o->timer++;

    switch (o->state) {
    case BUDPLANT_ST_IDLE:
        if (50 <= o->timer && (rng_i32() < 0x3000000U || 150 <= o->timer)) {
            o->timer = 0, o->state++;
        }
        break;
    case BUDPLANT_ST_PREPARING:
        if (BUDPLANT_TICKS_PREPARE <= o->timer) {
            o->timer = 0, o->state++;
        }
        break;
    case BUDPLANT_ST_SHAKING:
        if (BUDPLANT_TICKS_SHAKING <= o->timer) {
            o->timer = 0, o->state++;
        }
        break;
    case BUDPLANT_ST_SHOOTING: {
#define BUDPLANT_PROJ_OFFS 32

        if ((BUDPLANT_TICKS_SHOOTING * 13) / 16 == o->timer) {
            v2_i32 ppos = obj_pos_center(o);
            v2_i32 pv   = {rngr_sym_i32(600), -rngr_i32(1400, 1700)};
            ppos.y -= BUDPLANT_PROJ_OFFS;

            bpgrenade_create(g, ppos, pv);
            f32 vol = cam_snd_scale(g, o->pos, 300);
            snd_play(SNDID_PROJECTILE_SPIT, vol, rngr_f32(0.9f, 1.1f));
        }
        if (BUDPLANT_TICKS_SHOOTING <= o->timer) {
            o->timer = 0;
            o->state = BUDPLANT_ST_IDLE;
        }
        break;
    }
    default: break;
    }
}

void budplant_on_animate(g_s *g, obj_s *o)
{
    budplant_s   *bp  = (budplant_s *)o->mem;
    obj_sprite_s *spr = &o->sprites[0];

    spr->offs.x = -(64 - o->w) / 2;
    spr->offs.y = -60 + o->h;
    i32 imgx    = 0;
    i32 imgy    = 0;

    switch (o->state) {
    default: break;
    case BUDPLANT_ST_IDLE: {
        imgy = 0;
        imgx = (o->timer >> 3) & 3;
        break;
    }
    case BUDPLANT_ST_PREPARING: {
        imgy = 1;
        imgx = lerp_i32(0, 8, o->timer, BUDPLANT_TICKS_PREPARE);
        imgx = min_i32(imgx, 7);
        break;
    }
    case BUDPLANT_ST_SHAKING: {
        imgy = 2;
        imgx = (o->timer / 3) & 1;
        break;
    }
    case BUDPLANT_ST_SHOOTING: {
        imgy = 3;
        imgx = lerp_i32(0, 4, o->timer, BUDPLANT_TICKS_SHOOTING);
        imgx = min_i32(imgx, 2);
        break;
    }
    }

    if (o->enemy.hurt_tick) {
        imgx = 4;
        imgy = 0;
    }

    spr->trec = asset_texrec(TEXID_BUDPLANT, imgx << 6, imgy << 6, 64, 64);
}

void budplant_on_hurt(g_s *g, obj_s *o)
{
    o->state = BUDPLANT_ST_PREPARING;
    o->timer = 0;
}