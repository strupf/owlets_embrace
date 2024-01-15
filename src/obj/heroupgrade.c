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
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_RENDER_AABB;
    o->w = 16;
    o->h = 16;

    o->n_sprites         = 1;
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(TEXID_MISCOBJ, 0, 0, 32, 32);
    spr->offs.x          = -8;
    spr->offs.y          = -8;

    return o;
}

obj_s *heroupgrade_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = heroupgrade_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y;

    char prop[64] = {0};
    map_obj_strs(mo, "Heroupgrade", prop);
    if (0) {
    } else if (str_eq(prop, "HIJUMP")) {
        o->state = HERO_UPGRADE_HIGH_JUMP;
    }

    return o;
}

void heroupgrade_on_collect(game_s *g, obj_s *o, herodata_s *h)
{
    hero_aquire_upgrade(h, o->state);
    fade_start(&g->fade_upgrade, 60, 200, 60, NULL, NULL, NULL);
}

void heroupgrade_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
}