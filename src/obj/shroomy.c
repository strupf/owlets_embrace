// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    SHROOMY_WALK,
    SHROOMY_HIDE,
    SHROOMY_HIDDEN,
    SHROOMY_APPEAR,
};

static const frame_ticks_s g_shroomyhide = {30, 7, 3, 2, 2, 2, 2, 2};

#define SHROOMY_TICKS_APPEAR 10
#define SHROOMY_TICKS_HIDDEN 50

obj_s *shroomy_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_SHROOMY;
    o->flags |= OBJ_FLAG_ACTOR;
    o->flags |= OBJ_FLAG_MOVER;
    o->flags |= OBJ_FLAG_KILL_OFFSCREEN;
    o->flags |= OBJ_FLAG_SPRITE;
    o->facing       = 1;
    o->gravity_q8.y = 30;
    o->drag_q8.y    = 255;
    o->drag_q8.x    = 256;
    o->w            = 16;
    o->h            = 16;
    o->moverflags |= OBJ_MOVER_SLOPES;
    o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;

    o->n_sprites         = 1;
    sprite_simple_s *spr = &o->sprites[0];

    spr->trec   = asset_texrec(TEXID_SHROOMY, 0, 0, 64, 48);
    spr->offs.x = -(spr->trec.r.w - o->w) / 2;
    spr->offs.y = -(spr->trec.r.h - o->h);
    return o;
}

void shroomy_on_update(game_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = 0;
    }

    v2_i32 off = {o->facing, 0};
    if ((o->bumpflags & OBJ_BUMPED_X) ||
        (obj_grounded(g, o) && !obj_grounded_at_offs(g, o, off))) {
        o->facing = -o->facing;
    }

    o->bumpflags = 0;

    switch (o->state) {
    case SHROOMY_WALK: {
        o->vel_q8.x = o->facing * 64;
        if (inp_debug_space()) { // saw hero
            o->state    = SHROOMY_HIDE;
            o->timer    = 0;
            o->vel_q8.x = 0;
        }
    } break;
        //
    case SHROOMY_HIDE:
        if (anim_total_ticks(&g_shroomyhide) <= ++o->timer) {
            o->state = SHROOMY_HIDDEN;
            o->timer = 0;
        }
        break;
        //
    case SHROOMY_HIDDEN:
        if (++o->timer < SHROOMY_TICKS_HIDDEN) {
            break;
        }

        if (inp_debug_space()) { // no danger in sight
            o->state = SHROOMY_APPEAR;
            o->timer = 0;
        }
        break;
        //
    case SHROOMY_APPEAR:
        if (SHROOMY_TICKS_APPEAR <= ++o->timer)
            o->state = SHROOMY_WALK;
        break;
    }
}

void shroomy_on_animate(game_s *g, obj_s *o)
{
    sprite_simple_s *spr = &o->sprites[0];
    spr->flip            = o->facing == 1 ? SPR_FLIP_X : 0;

    const int H = spr->trec.r.h;
    const int W = spr->trec.r.w;

    switch (o->state) {
        //
    case SHROOMY_WALK: {
        o->animation += o->vel_q8.x;
        spr->trec.r.y = 0;
        if (obj_grounded(g, o))
            spr->trec.r.x = W * ((o->animation >> 9) & 3);
        else
            spr->trec.r.x = 0;
    } break;
        //
    case SHROOMY_HIDE: {

        spr->trec.r.y = 1 * H;
        spr->trec.r.x = W * anim_frame_from_ticks(o->timer, &g_shroomyhide);
    } break;
        //
    case SHROOMY_HIDDEN: {
        spr->trec.r.y = 2 * H;
        spr->trec.r.x = (o->timer >= SHROOMY_TICKS_HIDDEN) * W;
    } break;
        //
    case SHROOMY_APPEAR: {
        spr->trec.r.y = 3 * H;
        spr->trec.r.x = lerp_i32(0, 3,
                                 o->timer,
                                 SHROOMY_TICKS_APPEAR) *
                        W;
    } break;
    }
}