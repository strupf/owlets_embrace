// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    obj_handle_s parent;
    i32          n_frames;
    u16          t;
    u16          t_og;
    u16          x;
    u16          y;
    u16          w;
    u16          h;
} spritedecal_s;

void spritedecal_on_update(g_s *g, obj_s *o)
{
    spritedecal_s *sd = (spritedecal_s *)o->mem;
    sd->t++;
    if (sd->t_og <= sd->t) {
        obj_delete(g, o);
    }
    obj_move_by_v_q8(g, o);
    o->v_q8.y += 50;
}

void spritedecal_on_animate(g_s *g, obj_s *o)
{
    spritedecal_s *sd     = (spritedecal_s *)o->mem;
    obj_sprite_s  *spr    = &o->sprites[0];
    obj_s         *parent = obj_from_obj_handle(sd->parent);
    if (parent) {
        o->pos = parent->pos;
    }
    spr->trec.x = sd->x + sd->w * ((sd->t * sd->n_frames) / sd->t_og);
}

obj_s *spritedecal_create(g_s *g, i32 render_priority, obj_s *oparent, v2_i32 pos,
                          i32 texID, rec_i32 srcr, i32 ticks, i32 n_frames, i32 flip)
{
    obj_s         *o   = obj_create(g);
    spritedecal_s *sd  = (spritedecal_s *)o->mem;
    o->ID              = OBJID_SPRITEDECAL;
    o->flags           = OBJ_FLAG_SPRITE;
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
