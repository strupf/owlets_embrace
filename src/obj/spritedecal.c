// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    obj_handle_s parent;
    i16          dir;
    i16          n_frames;
    u16          t;
    u16          t_og;
    u16          x;
    u16          y;
    u16          w;
    u16          h;
} spritedecal_s;

void spritedecal_on_update(g_s *g, obj_s *o);
void spritedecal_on_animate(g_s *g, obj_s *o);

void spritedecal_on_update(g_s *g, obj_s *o)
{
    spritedecal_s *sd = (spritedecal_s *)o->mem;
    sd->t++;
    if (sd->t_og <= sd->t) {
        obj_delete(g, o);
        return;
    }

    obj_move_by_v_q12(g, o);

    obj_sprite_s *spr    = &o->sprites[0];
    obj_s        *parent = obj_from_obj_handle(sd->parent);
    if (parent) {
        o->pos = parent->pos;
    }
    if (sd->dir == 0) {
        spr->trec.x = sd->x + sd->w * ((sd->t * sd->n_frames) / sd->t_og);
    } else {
        spr->trec.y = sd->y + sd->h * ((sd->t * sd->n_frames) / sd->t_og);
    }
}

void spritedecal_on_animate(g_s *g, obj_s *o)
{
}

obj_s *spritedecal_create(g_s *g, i32 render_priority, obj_s *oparent, v2_i32 pos,
                          i32 texID, rec_i32 srcr, i32 ticks, i32 n_frames, i32 flip)
{
    obj_s         *o   = obj_create(g);
    spritedecal_s *sd  = (spritedecal_s *)o->mem;
    o->ID              = OBJID_SPRITEDECAL;
    o->n_sprites       = 1;
    o->render_priority = render_priority;
    obj_sprite_s *spr  = &o->sprites[0];
    spr->flip          = flip;
    if (oparent) {
        sd->parent = obj_handle_from_obj(oparent);
        spr->offs  = v2_i16_from_i32(pos);
    } else {
        o->pos = pos;
    }

    sd->n_frames = n_frames;
    sd->x        = srcr.x;
    sd->y        = srcr.y;
    sd->w        = srcr.w;
    sd->h        = srcr.h;
    sd->t_og     = ticks;

    spr->trec = asset_texrec(texID, sd->x, sd->y, sd->w, sd->h);
    return o;
}

obj_s *objanim_create(g_s *g, v2_i32 p, i32 objanimID)
{
    obj_s         *o  = obj_create(g);
    spritedecal_s *sd = (spritedecal_s *)o->mem;
    o->ID             = OBJID_SPRITEDECAL;
    o->n_sprites      = 1;
    o->on_update      = spritedecal_on_update;

    obj_sprite_s *spr   = &o->sprites[0];
    i32           texID = 0;

    switch (objanimID) {
    case OBJANIMID_ENEMY_EXPLODE:
        texID              = TEXID_PARTICLES;
        spr->flip          = gfx_spr_flip_rng(1, 1);
        o->render_priority = RENDER_PRIO_HERO - 1;
        sd->n_frames       = 14;
        sd->x              = 0;
        sd->y              = 320;
        sd->w              = 64;
        sd->h              = 64;
        sd->t_og           = 30;
        break;
    case OBJANIMID_EXPLODE_GRENADE:
        texID              = TEXID_EXPLO1;
        spr->flip          = gfx_spr_flip_rng(1, 1);
        o->render_priority = RENDER_PRIO_HERO - 1;
        sd->dir            = 1;
        sd->n_frames       = 13;
        sd->x              = 0;
        sd->y              = 0;
        sd->w              = 128;
        sd->h              = 128;
        sd->t_og           = 40;
        break;
    case OBJANIMID_STOMP_R:
    case OBJANIMID_STOMP_L:
    case OBJANIMID_STOMP_R_STRONG:
    case OBJANIMID_STOMP_L_STRONG:
        texID = TEXID_PARTICLES;
        if (objanimID == OBJANIMID_STOMP_R ||
            objanimID == OBJANIMID_STOMP_R_STRONG) {
            spr->flip = SPR_FLIP_X;
            p.x += 35;
        } else {
            p.x -= 35;
        }
        p.y -= 22;
        o->render_priority = RENDER_PRIO_HERO + 1;

        sd->x = 0;
        if (objanimID == OBJANIMID_STOMP_R_STRONG ||
            objanimID == OBJANIMID_STOMP_L_STRONG) {
            sd->y        = 896;
            sd->n_frames = 13 - 1;
            sd->x        = 64;
        } else {
            sd->y        = 576;
            sd->n_frames = 9;
        }

        sd->t_og = sd->n_frames * 2;
        sd->dir  = 0;
        sd->w    = 64;
        sd->h    = 64;
        break;
    case OBJANIM_BOULDER_POOF:
        texID              = TEXID_PARTICLES;
        spr->flip          = gfx_spr_flip_rng(1, 1);
        o->render_priority = RENDER_PRIO_HERO - 1;
        sd->n_frames       = 9 - 1;
        sd->x              = 1 * 64;
        sd->y              = 192;
        sd->w              = 64;
        sd->h              = 64;
        sd->t_og           = 20;
        break;
    }

    o->pos      = p;
    spr->offs.x = -sd->w / 2;
    spr->offs.y = -sd->h / 2;
    spr->trec   = asset_texrec(texID, sd->x, sd->y, sd->w, sd->h);
    return o;
}