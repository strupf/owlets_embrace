// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    BUDPLANT_ST_IDLE,
    BUDPLANT_ST_PREPARING,
    BUDPLANT_ST_SHAKING,
    BUDPLANT_ST_SHOOTING,
    BUDPLANT_ST_POP_OUT,
    BUDPLANT_ST_WALK,
};

enum {
    BUDPLANT_SUBID_STATIONARY,
    BUDPLANT_SUBID_PROJECTILE,
};

typedef struct {
    i32 cooldown;
} budplant_s;

#define BUDPLANT_TICKS_PREPARE  25
#define BUDPLANT_TICKS_SHAKING  20
#define BUDPLANT_TICKS_SHOOTING 25
#define BUDPLANT_TICKS_POP_OUT  16

void bpgrenade_create(g_s *g, v2_i32 pos, v2_i32 vel);
//
void budplant_on_hurt(g_s *g, obj_s *o);
void budplant_on_update(g_s *g, obj_s *o);
void budplant_on_animate(g_s *g, obj_s *o);

void budplant_load(g_s *g, map_obj_s *mo)
{
    obj_s      *o  = obj_create(g);
    budplant_s *bp = (budplant_s *)o->mem;
    o->editorUID   = mo->UID;
    o->ID          = OBJID_BUDPLANT;
    o->flags       = OBJ_FLAG_ACTOR |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY;
    o->subID              = BUDPLANT_SUBID_STATIONARY;
    o->moverflags         = OBJ_MOVER_TERRAIN_COLLISIONS;
    o->on_update          = budplant_on_update;
    o->on_animate         = budplant_on_animate;
    o->w                  = 24;
    o->h                  = 24;
    o->pos.x              = mo->x + (mo->w - o->w) / 2;
    o->pos.y              = mo->y + (mo->h - o->h);
    o->health_max         = 3;
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
    budplant_s *bp    = (budplant_s *)o->mem;
    obj_s      *ohero = obj_get_owl(g);
    o->timer++;

    switch (o->state) {
    case BUDPLANT_ST_IDLE: {
        if (bp->cooldown) {
            bp->cooldown--;
            break;
        }

        if (ohero && obj_dist_appr(o, ohero) < 100) {
            o->subtimer++;
        } else if (o->subtimer) {
            o->subtimer--;
        }

        if (35 <= o->subtimer) {
            o->timer    = 0;
            o->state    = BUDPLANT_ST_SHAKING;
            o->subtimer = 0;
        }
        break;
    }
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
        if (o->timer == 10) {
            v2_i32 oanim = obj_pos_center(o);
            oanim.y -= 32;

            f32 vol = 0.f;
            sfx_cuef(SFXID_EXPLOPOOF, vol, rngr_f32(0.9f, 1.1f));
        }
        if (BUDPLANT_TICKS_SHOOTING <= o->timer) {
            o->timer     = 0;
            o->state     = BUDPLANT_ST_IDLE;
            bp->cooldown = 35;
        }
        break;
    }
    case BUDPLANT_ST_POP_OUT: {
        if (BUDPLANT_TICKS_POP_OUT <= o->timer) {
            o->state = BUDPLANT_ST_WALK;
            o->timer = 0;
        }
        break;
    }
    case BUDPLANT_ST_WALK: {
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
        if (o->subtimer) {
            if (20 <= o->subtimer) {
                imgy = 2;
                imgx = (o->timer / 3) & 1;
            } else {
                imgy = 1;
                imgx = lerp_i32(0, 8, o->subtimer, 20);
            }
        } else {
            imgy = 0;
            imgx = (o->timer >> 3) & 3;
        }
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

        if (o->timer < 6) {
            imgx += 2;
        }
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