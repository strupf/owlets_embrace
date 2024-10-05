// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_powerup_obj_load(game_s *g, map_obj_s *mo)
{
    i32 upgrade = map_obj_i32(mo, "ID");
    // if (hero_has_upgrade(g, upgrade)) return;

    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_HERO_POWERUP;
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

void hero_powerup_obj_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
}

i32 hero_powerup_obj_ID(obj_s *o)
{
    return o->state;
}