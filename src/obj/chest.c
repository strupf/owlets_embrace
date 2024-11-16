// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define CHEST_TICKS_OPEN 20

enum {
    CHEST_CLOSED,
    CHEST_OPENING,
    CHEST_OPENED,
};

void chest_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = 0;
    o->flags = OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_CAN_BE_JUMPED_ON;
    o->pos.x     = mo->x;
    o->pos.y     = mo->y;
    o->w         = mo->w;
    o->h         = mo->h;
    o->n_sprites = 1;
    o->mass      = 1;
}

void chest_on_update(g_s *g, obj_s *o)
{
    switch (o->state) {
    case CHEST_CLOSED: {
        if (!(o->bumpflags & OBJ_BUMP_JUMPED_ON)) break;
        o->state = CHEST_OPENING;
        break;
    }
    case CHEST_OPENING: {
        o->timer++;
        if (CHEST_TICKS_OPEN <= o->timer) {
            o->state = CHEST_OPENED;
        }
        break;
    }
    case CHEST_OPENED: {
        break;
    }
    }

    o->bumpflags = 0;
}

void chest_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];

    i32 animID  = 0;
    i32 frameID = 0;

    switch (o->state) {
    case CHEST_CLOSED: {
        break;
    }
    case CHEST_OPENING: {
        break;
    }
    case CHEST_OPENED: {
        break;
    }
    }

    spr->trec = asset_texrec(0, frameID * 64, animID * 64, 64, 64);
}