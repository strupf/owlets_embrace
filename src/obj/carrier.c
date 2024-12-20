// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void carrier_on_update(g_s *g, obj_s *o)
{
    bool32 bumpedx = (o->bumpflags & OBJ_BUMP_X);
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q8.y = 0;
    }
    if (bumpedx) {
        o->v_q8.x = 0;
    }
    o->bumpflags = 0;

    obj_s *hero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!hero) return;

    rec_i32 rtop      = {o->pos.x, o->pos.y, o->w, 1};
    rec_i32 rherofeet = obj_rec_bottom(hero);
    if (!overlap_rec(rherofeet, rtop)) return;

    if (bumpedx) {
        o->v_q8.y = -1000;
    }

    if (obj_grounded(g, o)) {
        o->v_q8.x += hero->facing * 50;
    } else {
        o->v_q8.x += hero->facing * 15;
    }
}

void carrier_on_animate(g_s *g, obj_s *o)
{
    o->sprites[0].flip = o->v_q8.x < 0 ? SPR_FLIP_X : 0;
}

void carrier_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_CARRIER;
    o->flags = OBJ_FLAG_PLATFORM |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_KILL_OFFSCREEN |
               // OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_SPRITE;

    o->moverflags = OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_SLIDE_Y_NEG |
                    OBJ_MOVER_GLUE_GROUND;
    o->on_update       = carrier_on_update;
    o->on_animate      = carrier_on_animate;
    o->w               = 40;
    o->h               = 24;
    o->render_priority = 2;

    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec         = asset_texrec(TEXID_CARRIER, 0, 0, 96, 48);
    spr->offs.y       = o->h - 48;
    spr->offs.x       = -24;
    o->pos.x          = mo->x;
    o->pos.y          = mo->y - o->h;
}
