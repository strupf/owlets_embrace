// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "hero.h"

obj_s *heroupgrade_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HEROUPGRADE;
    o->flags = OBJ_FLAG_COLLECTIBLE |
               OBJ_FLAG_RENDER_AABB;
    o->w = 16;
    o->h = 16;

    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(0, 0, 0, 0, 0);

    return o;
}

void heroupgrade_on_collect(game_s *g, obj_s *o, herodata_s *h)
{
    hero_aquire_upgrade(h, o->state);
}

void heroupgrade_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(0, 0, 0, 0, 0);

    gfx_ctx_s ctx = gfx_ctx_display();
    // rec_i32   rr  = {o->pos.x + cam.x, o->pos.y + cam.y, o->}
}