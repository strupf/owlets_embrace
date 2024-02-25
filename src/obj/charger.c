// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// charger enemy: walks around; charges towards the player

#include "game.h"

enum {
    CHARGER_STATE_NORMAL,
    CHARGER_STATE_CHARGING,
    CHARGER_STATE_STUNNED,
};

#define CHARGER_TICKS_CHARGE       100
#define CHARGER_TICKS_STATE_CHANGE 50
#define CHARGER_TICKS_STUNNED      100
#define CHARGER_TRIGGER_AREA_W     100
#define CHARGER_TRIGGER_AREA_H     50

static void charger_update_normal(game_s *g, obj_s *o);
static void charger_update_charging(game_s *g, obj_s *o);
static void charger_update_stunned(game_s *g, obj_s *o);

obj_s *charger_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_CHARGER;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
               // OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_CLAMP_ROOM_X |
#if GAME_JUMP_ATTACK
               OBJ_FLAG_PLATFORM | OBJ_FLAG_PLATFORM_HERO_ONLY |
#endif
               OBJ_FLAG_KILL_OFFSCREEN;
    o->w                 = 32;
    o->h                 = 32;
    o->gravity_q8.y      = 80;
    o->drag_q8.y         = 255;
    o->drag_q8.x         = 256;
    o->moverflags        = OBJ_MOVER_GLUE_GROUND | OBJ_MOVER_SLOPES;
    o->vel_cap_q8.x      = 2000;
    o->facing            = -1;
    o->health_max        = 3;
    o->health            = o->health_max;
    o->enemy             = enemy_default();
    o->n_sprites         = 1;
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(TEXID_MISCOBJ, 336, 16, 80, 64);
    spr->offs.x          = -30;
    spr->offs.y          = o->h - spr->trec.r.h;
    return o;
}

void charger_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = charger_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y;
}

static void charger_update_normal(game_s *g, obj_s *o)
{
    bool32 bumpedx = (o->bumpflags & OBJ_BUMPED_X);
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = 0;
    }
    o->bumpflags = 0;

    o->subtimer++;
    if (o->subtimer < CHARGER_TICKS_STATE_CHANGE) return;

    o->timer++;

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero && obj_grounded(g, o)) {
        const rec_i32 aabb     = obj_aabb(o);
        rec_i32       rtrigger = {0};
        rtrigger.w             = CHARGER_TRIGGER_AREA_W;
        rtrigger.h             = CHARGER_TRIGGER_AREA_H;
        rtrigger.y             = aabb.y + aabb.h - CHARGER_TRIGGER_AREA_H;
        if (o->facing == 1) {
            rtrigger.x = aabb.x + aabb.w;
        } else {
            rtrigger.x = aabb.x - CHARGER_TRIGGER_AREA_W;
        }

        if (overlap_rec(rtrigger, obj_aabb(ohero))) {
            o->state    = CHARGER_STATE_CHARGING;
            o->timer    = 0;
            o->subtimer = 0;
            return;
        }
    }

    if (bumpedx || obj_would_fall_down_next(g, o, o->facing)) {
        o->facing = -o->facing;
    }

    if ((o->timer & 3) == 0) {
        o->tomove.x = o->facing;
    }
}

static void charger_update_charging(game_s *g, obj_s *o)
{
    flags32 bflags  = o->bumpflags;
    bool32  bumpedx = (o->bumpflags & OBJ_BUMPED_X);
    bool32  bumpedy = (o->bumpflags & OBJ_BUMPED_Y);
    o->bumpflags    = 0;
    if (bumpedy) {
        o->vel_q8.y = 0;
    }

    if (bumpedx) {
        snd_play_ext(SNDID_CRUMBLE, 0.5f, 2.f);
        cam_screenshake(&g->cam, 10, 5);
        o->vel_q8.x = 0;
        if (!obj_grounded(g, o)) {
            o->vel_q8.y = -400;
        }
        if (bflags & OBJ_BUMPED_X_NEG) o->vel_q8.x = +150;
        if (bflags & OBJ_BUMPED_X_POS) o->vel_q8.x = -150;
        o->state    = CHARGER_STATE_STUNNED;
        o->subtimer = 0;
        return;
    }

    o->subtimer++;
    if (o->subtimer < CHARGER_TICKS_STATE_CHANGE) return;

    o->timer++;
    if (obj_grounded(g, o) && (bumpedx || CHARGER_TICKS_CHARGE <= o->timer)) {
        o->state    = CHARGER_STATE_NORMAL;
        o->timer    = 0;
        o->subtimer = 0;
        return;
    }

    o->vel_q8.x += o->facing * 64;
}

static void charger_update_stunned(game_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = -(o->vel_q8.y >> 1);
    }
    if (o->bumpflags & OBJ_BUMPED_X) {
        o->vel_q8.x = 0;
    } else if (obj_grounded(g, o)) {
        o->vel_q8.x /= 2;
    }

    o->bumpflags = 0;
    o->subtimer++;
    if (o->subtimer < CHARGER_TICKS_STUNNED) return;

    o->facing   = -o->facing;
    o->state    = CHARGER_STATE_NORMAL;
    o->subtimer = 0;
}

void charger_on_update(game_s *g, obj_s *o)
{
    switch (o->state) {
    case CHARGER_STATE_NORMAL: {
        charger_update_normal(g, o);
    } break;
    case CHARGER_STATE_CHARGING: {
        charger_update_charging(g, o);
    } break;
    case CHARGER_STATE_STUNNED: {
        charger_update_stunned(g, o);
    } break;
    }
}

void charger_on_animate(game_s *g, obj_s *o)
{
    sprite_simple_s *spr = &o->sprites[0];
    spr->flip            = o->facing == 1 ? SPR_FLIP_X : 0;
    switch (o->state) {
    case CHARGER_STATE_NORMAL: {
        if (o->subtimer < CHARGER_TICKS_STATE_CHANGE) {

        } else {
        }
    } break;
    case CHARGER_STATE_CHARGING: {
        if (o->subtimer < CHARGER_TICKS_STATE_CHANGE) {

        } else {
        }
    } break;
    }
}