// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// charger enemy: walks around; charges towards the player

#include "game.h"

enum {
    CHARGER_STATE_NORMAL,
    CHARGER_STATE_CHARGING,
};

#define CHARGER_TICKS_CHARGE       100
#define CHARGER_TICKS_STATE_CHANGE 50
#define CHARGER_TRIGGER_AREA_W     100
#define CHARGER_TRIGGER_AREA_H     50

static void charger_update_normal(game_s *g, obj_s *o);
static void charger_update_charging(game_s *g, obj_s *o);

obj_s *charger_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_CHARGER;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_RENDER_AABB;
    o->w            = 32;
    o->h            = 32;
    o->gravity_q8.y = 30;
    o->drag_q8.y    = 255;
    o->moverflags   = OBJ_MOVER_GLUE_GROUND | OBJ_MOVER_SLOPES;
    o->facing       = (rngr_i32(0, 1) << 1) - 1; // -1 or +1

    o->n_sprites         = 1;
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(TEXID_CRAWLER, 0, 0, 32, 32);
    return o;
}

void charger_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = charger_create(g);
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

    if (bumpedx || !(obj_grounded_at_offs(g, o, (v2_i32){o->facing, 0}) ||
                     obj_grounded_at_offs(g, o, (v2_i32){o->facing, 1}))) {
        o->facing = -o->facing;
    }

    if ((o->timer & 3) == 0) {
        o->tomove.x = o->facing;
    }
}

static void charger_update_charging(game_s *g, obj_s *o)
{
    bool32 bumpedx = (o->bumpflags & OBJ_BUMPED_X);
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = 0;
    }
    o->bumpflags = 0;

    o->subtimer++;
    if (o->subtimer < CHARGER_TICKS_STATE_CHANGE) return;

    o->timer++;
    if (obj_grounded(g, o) && (bumpedx || CHARGER_TICKS_CHARGE <= o->timer)) {
        o->state    = CHARGER_STATE_NORMAL;
        o->timer    = 0;
        o->subtimer = 0;
        return;
    }

    o->tomove.x = o->facing * 2;
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
    }
}

void charger_on_animate(game_s *g, obj_s *o)
{
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