// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    SHROOMY_WALK,
    SHROOMY_HIDE,
    SHROOMY_HIDDEN,
    SHROOMY_BOUNCED,
    SHROOMY_APPEAR,
};

static const frame_ticks_s g_shroomyhide = {{30, 7, 3, 2, 2, 2, 2, 2}};

#define SHROOMY_TICKS_APPEAR   10
#define SHROOMY_TICKS_HIDDEN   50
#define SHROOMY_NOTICE_RANGE_X 100

obj_s *shroomy_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_SHROOMY;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_SPRITE;
    o->facing       = 1;
    o->gravity_q8.y = 30;
    o->drag_q8.y    = 255;
    o->drag_q8.x    = 200;
    o->w            = 16;
    o->h            = 16;
    o->moverflags =
        OBJ_MOVER_SLOPES |
        OBJ_MOVER_ONE_WAY_PLAT;

    o->n_sprites         = 1;
    sprite_simple_s *spr = &o->sprites[0];

    spr->trec   = asset_texrec(TEXID_SHROOMY, 0, 0, 64, 48);
    spr->offs.x = -(spr->trec.r.w - o->w) / 2;
    spr->offs.y = -(spr->trec.r.h - o->h);
    return o;
}

void shroomy_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = shroomy_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y - mo->h;
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

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);

    rec_i32 rr;
    if (o->facing == -1) {
        rr = obj_rec_left(o);
        rr.w += SHROOMY_NOTICE_RANGE_X;
        rr.x -= SHROOMY_NOTICE_RANGE_X;
    } else {
        rr = obj_rec_right(o);
        rr.w += SHROOMY_NOTICE_RANGE_X;
    }

    switch (o->state) {
    case SHROOMY_WALK: {
        o->vel_q8.x = o->facing * 64;

        if (ohero && overlap_rec(obj_aabb(ohero), rr)) { // saw hero
            o->state    = SHROOMY_HIDE;
            o->timer    = 0;
            o->vel_q8.x = 0;
        }
    } break;
    case SHROOMY_HIDE:
        if (anim_total_ticks((frame_ticks_s *)&g_shroomyhide) <= ++o->timer) {
            o->state = SHROOMY_HIDDEN;
            o->timer = 0;
        }
        break;
    case SHROOMY_HIDDEN:
        if (++o->timer < SHROOMY_TICKS_HIDDEN) {
            break;
        }

        if (!ohero || !overlap_rec(obj_aabb(ohero), rr)) { // no danger in sight
            o->state = SHROOMY_APPEAR;
            o->timer = 0;
        }
        break;
    case SHROOMY_BOUNCED: {
        o->timer++;
        if (o->timer == 60) {
            o->state = SHROOMY_HIDDEN;
            o->timer = 0;
        }
    } break;
    case SHROOMY_APPEAR:
        if (SHROOMY_TICKS_APPEAR <= ++o->timer)
            o->state = SHROOMY_WALK;
        break;
    }
}

void shroomy_bounced_on(obj_s *o)
{
    snd_play_ext(SNDID_SHROOMY_JUMP, 0.15f, 1.f);
    o->state = SHROOMY_BOUNCED;
    o->timer = 0;
}

void shroomy_on_animate(game_s *g, obj_s *o)
{
    sprite_simple_s *spr = &o->sprites[0];
    spr->flip            = o->facing == 1 ? SPR_FLIP_X : 0;

    const int H = spr->trec.r.h;
    const int W = spr->trec.r.w;

    switch (o->state) {
    case SHROOMY_WALK:
        o->animation += o->vel_q8.x;
        spr->trec.r.y = 0;
        if (obj_grounded(g, o))
            spr->trec.r.x = W * ((o->animation >> 9) & 3);
        else
            spr->trec.r.x = 0;
        break;
    case SHROOMY_HIDE:
        spr->trec.r.y = 1 * H;
        spr->trec.r.x = W * anim_frame_from_ticks(o->timer, (frame_ticks_s *)&g_shroomyhide);
        break;
    case SHROOMY_BOUNCED:
        spr->trec.r.y = 1 * H;
        spr->trec.r.x = W * (6 + tick_to_index_freq(o->timer, 2, 10));
        break;
    case SHROOMY_HIDDEN:
        spr->trec.r.y = 2 * H;
        spr->trec.r.x = (o->timer >= SHROOMY_TICKS_HIDDEN) * W;
        break;
    case SHROOMY_APPEAR:
        spr->trec.r.y = 3 * H;
        spr->trec.r.x = W * lerp_i32(0, 3, o->timer, SHROOMY_TICKS_APPEAR);
        break;
    }
}