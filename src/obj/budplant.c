// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    BUDPLANT_ORIENTATION_GROUND,
    BUDPLANT_ORIENTATION_LEFT_WALL,
    BUDPLANT_ORIENTATION_CEILING,
    BUDPLANT_ORIENTATION_RIGHT_WALL
};

enum {
    BUDPLANT_ANIM_IDLE,
    BUDPLANT_ANIM_PREPARING,
    BUDPLANT_ANIM_SHAKING,
    BUDPLANT_ANIM_SHOOTING
};

enum {
    BUDPLANT_ST_IDLE,
    BUDPLANT_ST_PREPARING,
    BUDPLANT_ST_SHAKING,
    BUDPLANT_ST_SHOOTING
};

typedef struct {
    i32 orientation;
} budplant_s;

#define BUDPLANT_TICKS_PREPARE  25
#define BUDPLANT_TICKS_SHAKING  50
#define BUDPLANT_TICKS_SHOOTING 25

void budplant_on_update(game_s *g, obj_s *o);
void budplant_on_animate(game_s *g, obj_s *o);

void budplant_load(game_s *g, map_obj_s *mo)
{
    obj_s      *o  = obj_create(g);
    budplant_s *bp = (budplant_s *)o->mem;

    o->ID    = OBJ_ID_BUDPLANT;
    o->flags = OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_CAN_BE_JUMPED_ON |
               OBJ_FLAG_PLATFORM_HERO_ONLY;
    o->w          = 16;
    o->h          = 16;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->on_update  = budplant_on_update;
    o->on_animate = budplant_on_animate;
    o->health_max = 1;
    o->health     = o->health_max;

    obj_sprite_s *spr = &o->sprites[0];
    o->n_sprites      = 1;

    for (i32 n = 0; n < 4; n++) {
        rec_i32 rr = {0};
        switch (n) {
        case BUDPLANT_ORIENTATION_GROUND:
            rr = obj_rec_bottom(o);
            break;
        case BUDPLANT_ORIENTATION_LEFT_WALL:
            rr = obj_rec_left(o);
            break;
        case BUDPLANT_ORIENTATION_CEILING:
            rr = obj_rec_top(o);
            break;
        case BUDPLANT_ORIENTATION_RIGHT_WALL:
            rr = obj_rec_right(o);
            break;
        }

        if (map_blocked(g, o, rr, 0)) {
            bp->orientation = n;
            break;
        }
    }
}

void budplant_on_update(game_s *g, obj_s *o)
{
    budplant_s *bp = (budplant_s *)o->mem;
    o->timer++;

    switch (o->state) {
    case BUDPLANT_ST_IDLE:
        if (50 <= o->timer && (rng_u32() < 0x3000000U || 150 <= o->timer)) {
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
            v2_i32 pv   = {0};

            switch (bp->orientation) {
            case BUDPLANT_ORIENTATION_GROUND:
                pv = (v2_i32){rngr_sym_i32(600), -2500};
                ppos.y -= BUDPLANT_PROJ_OFFS;
                break;
            case BUDPLANT_ORIENTATION_LEFT_WALL:
                pv = (v2_i32){2500, rngr_sym_i32(600)};
                ppos.x += BUDPLANT_PROJ_OFFS;
                break;
            case BUDPLANT_ORIENTATION_CEILING:
                pv = (v2_i32){rngr_sym_i32(600), 2500};
                ppos.y += BUDPLANT_PROJ_OFFS;
                break;
            case BUDPLANT_ORIENTATION_RIGHT_WALL:
                pv = (v2_i32){-2500, rngr_sym_i32(600)};
                ppos.x -= BUDPLANT_PROJ_OFFS;
                break;
            }

            obj_s *pr  = projectile_create(g, ppos, pv, PROJECTILE_ID_BUDPLANT);
            f32    vol = cam_snd_scale(g, o->pos, 300);
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

void budplant_on_animate(game_s *g, obj_s *o)
{
    budplant_s   *bp  = (budplant_s *)o->mem;
    obj_sprite_s *spr = &o->sprites[0];

    i32 frameID = 0;
    i32 animID  = BUDPLANT_ANIM_IDLE;

    switch (o->state) {
    case BUDPLANT_ST_IDLE: {
        animID  = BUDPLANT_ANIM_IDLE;
        frameID = (o->timer >> 3) & 3;
        break;
    }
    case BUDPLANT_ST_PREPARING: {
        animID  = BUDPLANT_ANIM_PREPARING;
        frameID = lerp_i32(0, 8, o->timer, BUDPLANT_TICKS_PREPARE);
        frameID = min_i32(frameID, 7);
        break;
    }
    case BUDPLANT_ST_SHAKING: {
        animID  = BUDPLANT_ANIM_SHAKING;
        frameID = (o->timer / 3) & 1;
        break;
    }
    case BUDPLANT_ST_SHOOTING: {
        animID  = BUDPLANT_ANIM_SHOOTING;
        frameID = lerp_i32(0, 4, o->timer, BUDPLANT_TICKS_SHOOTING);
        frameID = min_i32(frameID, 2);
        break;
    }
    default: break;
    }

    if (o->enemy.hurt_tick) {
        frameID = 4;
        animID  = 0;
    }

    frameID += bp->orientation * 8;

    switch (bp->orientation) {
    case BUDPLANT_ORIENTATION_GROUND:
        spr->offs.x = -(64 - o->w) / 2;
        spr->offs.y = -64 + o->h;
        break;
    case BUDPLANT_ORIENTATION_LEFT_WALL:
        spr->offs.x = 0;
        spr->offs.y = -(64 - o->h) / 2;
        break;
    case BUDPLANT_ORIENTATION_CEILING:
        spr->offs.x = -(64 - o->w) / 2;
        spr->offs.y = 0;
        break;
    case BUDPLANT_ORIENTATION_RIGHT_WALL:
        spr->offs.x = -64 + o->w;
        spr->offs.y = -(64 - o->h) / 2;
        break;
    }

    spr->trec = asset_texrec(TEXID_BUDPLANT,
                             frameID * 64,
                             animID * 64,
                             64,
                             64);
}