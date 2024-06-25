// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define FLOATER_BUMP_TIMER 20

typedef struct {
    u32 rng;
    i32 steer_ang_q16;
    i32 steer_change_q16;
    i32 vel_amount;
} floater_s;

void floater_on_update(game_s *g, obj_s *o);
void floater_on_animate(game_s *g, obj_s *o);

void floater_load(game_s *g, map_obj_s *mo)
{
    obj_s     *o  = obj_create(g);
    floater_s *fl = (floater_s *)o->mem;
    o->ID         = OBJ_ID_FLOATER;
    o->flags      = OBJ_FLAG_MOVER |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_PLATFORM_HERO_ONLY |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_CLAMP_TO_ROOM;
    o->on_update       = floater_on_update;
    o->on_animate      = floater_on_animate;
    o->w               = 24;
    o->h               = 24;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->health_max      = 1;
    o->health          = o->health_max;
    o->enemy           = enemy_default();
    o->drag_q8.y       = 256;
    o->drag_q8.x       = 256;
    o->render_priority = 1;
    o->n_sprites       = 1;
    fl->steer_ang_q16  = 20000;
}

void floater_on_update(game_s *g, obj_s *o)
{
    obj_s     *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    floater_s *fl    = (floater_s *)o->mem;

    o->timer++;
    if (o->subtimer) {
        o->subtimer++;
        if (FLOATER_BUMP_TIMER <= o->subtimer) {
            o->subtimer = 0;
            o->substate = 0;
        }
    }

    if (o->bumpflags) {
        o->subtimer = 1;
        if (o->bumpflags & OBJ_BUMPED_X) {
            if (o->bumpflags & OBJ_BUMPED_X_POS) {
                o->substate = OBJ_BUMPED_X_POS;
            }
            if (o->bumpflags & OBJ_BUMPED_X_NEG) {
                o->substate = OBJ_BUMPED_X_NEG;
            }
            o->vel_q8.x = -o->vel_q8.x;
        }
        if (o->bumpflags & OBJ_BUMPED_Y) {
            if (o->bumpflags & OBJ_BUMPED_Y_POS) {
                o->substate = OBJ_BUMPED_Y_POS;
            }
            if (o->bumpflags & OBJ_BUMPED_Y_NEG) {
                o->substate = OBJ_BUMPED_Y_NEG;
            }
            o->vel_q8.y = -o->vel_q8.y;
        }
    }
    if (o->bumpflags) {
        fl->steer_ang_q16 = rngs_u32_bound(&fl->rng, 0xFFFF);
    }

    fl->vel_amount       = 70;
    fl->steer_change_q16 = rngr_i32(-100, +100);
    fl->steer_ang_q16    = (fl->steer_ang_q16 + fl->steer_change_q16) & 0xFFFF;
    v2_i32 steer         = {(fl->vel_amount * sin_q16(fl->steer_ang_q16 << 2)) >> 16,
                            (fl->vel_amount * cos_q16(fl->steer_ang_q16 << 2)) >> 16};
    o->vel_q8            = steer;
    if (ohero) {
        rec_i32 herobot = obj_rec_bottom(ohero);
        rec_i32 rplat   = {o->pos.x, o->pos.y, o->w, 1};
        if (overlap_rec(herobot, rplat) && 0 < ohero->vel_q8.y) {
            o->vel_q8.y += 200;
            // fl->steer_ang_q16 -= sgn_i32(fl->steer_ang_q16) * 100;
        }
    }
    o->bumpflags = 0;
}

void floater_on_animate(game_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
    switch (o->substate) {
    case OBJ_BUMPED_X_POS: {
        break;
    }
    case OBJ_BUMPED_X_NEG: {
        break;
    }
    case OBJ_BUMPED_Y_POS: {
        break;
    }
    case OBJ_BUMPED_Y_NEG: {
        break;
    }
    default: break;
    }
}