// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
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

void shroomy_on_update(g_s *g, obj_s *o);
void shroomy_on_animate(g_s *g, obj_s *o);
void shroomy_bounced_on(obj_s *o);

void shroomy_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJID_SHROOMY;
    o->flags = OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_HERO_JUMPABLE;
    o->render_priority = RENDER_PRIO_HERO - 1;
    o->facing          = 1;
    o->w               = 16;
    o->h               = 16;
    o->moverflags      = OBJ_MOVER_SLIDE_Y_NEG |
                    OBJ_MOVER_GLUE_GROUND;

    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];

    spr->trec   = asset_texrec(TEXID_SHROOMY, 0, 0, 64, 48);
    spr->offs.x = -(spr->trec.w - o->w) / 2;
    spr->offs.y = -(spr->trec.h - o->h);
    o->pos.x    = mo->x;
    o->pos.y    = mo->y - mo->h;
}

void shroomy_on_update(g_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q8.y = 0;
    }

    if ((o->bumpflags & OBJ_BUMP_X) ||
        obj_would_fall_down_next(g, o, o->facing)) {
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
        o->v_q8.x = o->facing * 64;

        if (ohero && overlap_rec(obj_aabb(ohero), rr)) { // saw hero
            o->state  = SHROOMY_HIDE;
            o->timer  = 0;
            o->v_q8.x = 0;
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

void shroomy_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
    spr->flip         = o->facing == 1 ? SPR_FLIP_X : 0;

    const i32 H = spr->trec.h;
    const i32 W = spr->trec.w;

    switch (o->state) {
    case SHROOMY_WALK:
        o->animation += o->v_q8.x;
        spr->trec.y = 0;
        if (obj_grounded(g, o))
            spr->trec.x = W * ((o->animation >> 9) & 3);
        else
            spr->trec.x = 0;
        break;
    case SHROOMY_HIDE:
        spr->trec.y = 1 * H;
        spr->trec.x = W * anim_frame_from_ticks(o->timer, (frame_ticks_s *)&g_shroomyhide);
        break;
    case SHROOMY_BOUNCED:
        spr->trec.y = 1 * H;
        spr->trec.x = W * (6 + tick_to_index_freq(o->timer, 2, 10));
        break;
    case SHROOMY_HIDDEN:
        spr->trec.y = 2 * H;
        spr->trec.x = (o->timer >= SHROOMY_TICKS_HIDDEN) * W;
        break;
    case SHROOMY_APPEAR:
        spr->trec.y = 3 * H;
        spr->trec.x = W * lerp_i32(0, 3, o->timer, SHROOMY_TICKS_APPEAR);
        break;
    }
}

void shroomy_bounced_on(obj_s *o)
{
    snd_play(SNDID_SHROOMY_JUMP, 0.15f, 1.f);
    o->state = SHROOMY_BOUNCED;
    o->timer = 0;
}
