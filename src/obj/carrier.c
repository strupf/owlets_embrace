// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *carrier_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_CARRIER;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_PLATFORM |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_KILL_OFFSCREEN |
               // OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_SPRITE;

    o->moverflags = OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_SLOPES |
                    OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_AVOID_HEADBUMP;
    o->w               = 40;
    o->h               = 24;
    o->drag_q8.y       = 255;
    o->vel_cap_q8.x    = 2500;
    o->gravity_q8.y    = 60;
    o->render_priority = 2;

    o->n_sprites         = 1;
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(TEXID_CARRIER, 0, 0, 96, 48);
    spr->offs.y          = o->h - 48;
    spr->offs.x          = -24;
    return o;
}

void carrier_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = carrier_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y - o->h;
}

void carrier_on_update(game_s *g, obj_s *o)
{
    bool32 bumpedx = (o->bumpflags & OBJ_BUMPED_X);
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = 0;
    }
    if (bumpedx) {
        o->vel_q8.x = 0;
    }
    o->bumpflags = 0;
    o->drag_q8.x = 250;

    obj_s *hero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!hero) return;

    rec_i32 rtop      = {o->pos.x, o->pos.y, o->w, 1};
    rec_i32 rherofeet = obj_rec_bottom(hero);
    if (!overlap_rec(rherofeet, rtop)) return;

    if (bumpedx) {
        o->vel_q8.y = -1000;
    }

    if (obj_grounded(g, o)) {
        o->vel_q8.x += hero->facing * 50;
        o->drag_q8.x = 256;
    } else {
        o->vel_q8.x += hero->facing * 15;
    }
}

void carrier_on_animate(game_s *g, obj_s *o)
{
    o->sprites[0].flip = o->vel_q8.x < 0 ? SPR_FLIP_X : 0;
}