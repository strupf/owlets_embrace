// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void heroupgrade_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
}

void heroupgrade_load(game_s *g, map_obj_s *mo)
{
    int upgrade = map_obj_i32(mo, "Upgrade");
    if (hero_has_upgrade(g, upgrade)) return;

    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HEROUPGRADE;
    o->flags = OBJ_FLAG_COLLECTIBLE |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_RENDER_AABB;
    o->w = 16;
    o->h = 16;

    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec         = asset_texrec(TEXID_MISCOBJ, 0, 0, 32, 32);
    spr->offs.x       = -8;
    spr->offs.y       = -8;
    o->pos.x          = mo->x;
    o->pos.y          = mo->y;
    o->state          = upgrade;
}

void heroupgrade_on_collect(game_s *g, obj_s *o)
{
    hero_add_upgrade(g, o->state);
    if (o->state == HERO_UPGRADE_HOOK) {
        // hero_add_upgrade(g, HERO_UPGRADE_HOOK_LONG);
    }
    // substate_upgrade_collected(g, &g->substate, o->state);
    NOT_IMPLEMENTED
}
