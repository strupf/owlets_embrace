// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define CHEST_TICKS_OPEN 20

enum {
    CHEST_CLOSED,
    CHEST_OPENED,
};

void chest_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = 0;
    o->flags = OBJ_FLAG_SOLID |
               OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_SPRITE;
    o->pos.x     = mo->x;
    o->pos.y     = mo->y;
    o->w         = mo->w;
    o->h         = mo->h;
    o->n_sprites = 1;
}

void chest_on_update(game_s *g, obj_s *o)
{
    switch (o->state) {
    case CHEST_CLOSED: {
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!ohero) break;
        if (!overlap_rec(obj_aabb(o), obj_aabb(ohero))) break;
        o->state = CHEST_OPENED;
        break;
    }
    case CHEST_OPENED: {
        o->timer++;
        break;
    }
    }
}

void chest_on_animate(game_s *g, obj_s *o)
{
    sprite_simple_s *spr = &o->sprites[0];

    int animID  = 0;
    int frameID = 0;

    switch (o->state) {
    case CHEST_CLOSED: {
        break;
    }
    case CHEST_OPENED: {
        break;
    }
    }

    spr->trec = asset_texrec(0, frameID * 64, animID * 64, 64, 64);
}