// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 p0;
    v2_i32 p1;
} flyer_s;

void flyer_on_update(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
    o->timer++;
    flyer_s *f  = (flyer_s *)o->mem;
    i32      i  = cos_q16((o->timer << 10) + 0x20000) + 0x10000;
    v2_i32   p  = v2_lerp(f->p0, f->p1, i, 0x20000);
    i32      dx = sgn_i32(o->pos.x - p.x);
    o->pos      = p;

    if (dx != 0 && dx != o->state) {
        spr->flip = dx == -1 ? SPR_FLIP_X : 0;
        o->state  = dx;
    }
}

void flyer_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
#if 0
    int           fr  = ((o->timer >> 1) & 3) * 128;
    spr->trec         = asset_texrec(TEXID_FLYER, fr, 0, 128, 96);
#else
    o->animation++;

#define BUG_V 8000
    i32 wingID    = (o->animation >> 1) & 1;
    i32 sin1      = sin_q16(o->animation * BUG_V);
    i32 sin2      = sin_q16((o->animation * BUG_V) - 30000);
    i32 frameID   = (((o->animation * BUG_V + 10000) * 6) / 0x40000) % 6;
    frameID       = clamp_i32(frameID, 0, 5);
    spr->offs.y   = -60 + ((4 * sin1) >> 16);
    spr->offs.x   = -48;
    // pltf_log("%i\n", frameID);
    texrec_s tbug = asset_texrec(TEXID_FLYING_BUG, frameID * 96, (wingID + 1) * 96, 96, 96);
    spr->trec     = tbug;
    // gfx_spr(ctxbug, tbug, (v2_i32){150, 30 + boffs}, 0, 0);
#endif
}

void flyer_load(g_s *g, map_obj_s *mo)
{
    obj_s   *o = obj_create(g);
    flyer_s *f = (flyer_s *)o->mem;
    o->ID      = OBJID_FLYER;
    o->flags   = OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_KILL_OFFSCREEN;
    o->on_update  = flyer_on_update;
    o->on_animate = flyer_on_animate;
    o->w          = 24;
    o->h          = 24;
    o->health_max = 3;
    o->health     = o->health_max;
    o->enemy      = enemy_default();
    o->facing     = 1;
    o->n_sprites  = 1;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    f->p0.x       = mo->x;
    f->p0.y       = mo->y;
    v2_i16 p1     = map_obj_pt(mo, "P");
    f->p1.x       = p1.x << 4;
    f->p1.y       = p1.y << 4;

    obj_sprite_s *spr = &o->sprites[0];
    spr->offs.x       = -48;
    spr->offs.y       = -40;
}
