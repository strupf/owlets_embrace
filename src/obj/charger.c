// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
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

static void charger_update_normal(g_s *g, obj_s *o);
static void charger_update_charging(g_s *g, obj_s *o);
static void charger_update_stunned(g_s *g, obj_s *o);

static void charger_update_normal(g_s *g, obj_s *o)
{
    bool32 bumpedx = (o->bumpflags & OBJ_BUMP_X);
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q8.y = 0;
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

static void charger_update_charging(g_s *g, obj_s *o)
{
    flags32 bflags  = o->bumpflags;
    bool32  bumpedx = (o->bumpflags & OBJ_BUMP_X);
    bool32  bumpedy = (o->bumpflags & OBJ_BUMP_Y);
    o->bumpflags    = 0;
    if (bumpedy) {
        o->v_q8.y = 0;
    }

    if (bumpedx) {
        // snd_play(SNDID_CRUMBLE, 0.5f, 2.f);
        cam_screenshake(&g->cam, 10, 5);
        o->v_q8.x = 0;
        if (obj_grounded(g, o)) {
            o->v_q8.y = -700;
        }
        if (bflags & OBJ_BUMP_X_NEG) o->v_q8.x = +150;
        if (bflags & OBJ_BUMP_X_POS) o->v_q8.x = -150;
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

    o->v_q8.x += o->facing * 64;
}

static void charger_update_stunned(g_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q8.y = -(o->v_q8.y >> 1);
    }
    if (o->bumpflags & OBJ_BUMP_X) {
        o->v_q8.x = 0;
    } else if (obj_grounded(g, o)) {
        o->v_q8.x /= 2;
    }

    o->bumpflags = 0;
    o->subtimer++;
    if (o->subtimer < CHARGER_TICKS_STUNNED) return;

    o->facing   = -o->facing;
    o->state    = CHARGER_STATE_NORMAL;
    o->subtimer = 0;
}

void charger_on_update(g_s *g, obj_s *o)
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

void charger_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
    spr->flip         = o->facing == 1 ? 0 : SPR_FLIP_X;
    i32 animID        = 0;
    i32 frameID       = 0;

    switch (o->state) {
    case CHARGER_STATE_NORMAL: {
        animID  = 0;
        frameID = (o->subtimer >> 4) & 3;
        if (o->subtimer < CHARGER_TICKS_STATE_CHANGE) {

        } else {
        }
    } break;
    case CHARGER_STATE_CHARGING: {
        if (o->subtimer < CHARGER_TICKS_STATE_CHANGE) {
            animID  = 1;
            frameID = min_i32((o->subtimer >> 5), 1);
        } else {
            animID  = 2;
            frameID = (o->subtimer >> 2) & 3;
        }
    } break;
    }

    spr->trec = asset_texrec(TEXID_CHARGER, frameID * 128, animID * 64, 128, 64);
}

void charger_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_CHARGER;
    o->flags = OBJ_FLAG_MOVER |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
               // OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_CLAMP_ROOM_X |
               OBJ_FLAG_KILL_OFFSCREEN;
    o->on_update      = charger_on_update;
    o->on_animate     = charger_on_animate;
    o->w              = 48;
    o->h              = 32;
    o->grav_q8.y      = 80;
    o->drag_q8.y      = 255;
    o->drag_q8.x      = 256;
    o->moverflags     = OBJ_MOVER_GLUE_GROUND | OBJ_MOVER_SLIDE_Y_NEG;
    o->facing         = -1;
    o->health_max     = 3;
    o->health         = o->health_max;
    o->enemy          = enemy_default();
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec         = asset_texrec(TEXID_CHARGER, 0, 0, 128, 64);
    spr->offs.x       = -40;
    spr->offs.y       = o->h - spr->trec.r.h;
    o->pos.x          = mo->x;
    o->pos.y          = mo->y;
}
