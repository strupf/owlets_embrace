// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// charger enemy: walks around; charges towards the player

#include "game.h"

obj_s *charger_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_CHARGER;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_HURT_ON_TOUCH;
    o->w            = 16;
    o->h            = 16;
    o->gravity_q8.y = 30;
    o->drag_q8.x    = 250;
    o->drag_q8.y    = 255;

    o->n_sprites         = 1;
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(TEXID_CRAWLER, 0, 0, 32, 32);
    return o;
}

void charger_on_update(game_s *g, obj_s *o)
{
}

void charger_on_animate(game_s *g, obj_s *o)
{
}