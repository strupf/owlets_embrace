// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    CHEST_CLOSED,
    CHEST_OPENED,
};

typedef struct {
    u32 saveID;
} chest_s;

void chest_load(g_s *g, map_obj_s *mo)
{
    obj_s   *o   = obj_create(g);
    chest_s *c   = (chest_s *)o->mem;
    o->ID        = OBJ_ID_CHEST;
    o->pos.x     = mo->x;
    o->pos.y     = mo->y;
    o->w         = mo->w;
    o->h         = mo->h;
    o->n_sprites = 1;
    c->saveID    = map_obj_saveID(mo, "SaveID");
    if (saveID_has(g, c->saveID)) {
        o->state = CHEST_OPENED;
    } else {
        o->flags = OBJ_FLAG_HERO_JUMPABLE;
    }
}

void chest_on_update(g_s *g, obj_s *o)
{
}

void chest_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr     = &o->sprites[0];
    i32           frameID = 0;
    spr->offs.x           = (o->w - 48) >> 1;
    spr->offs.y           = o->h - 32;

    switch (o->state) {
    case CHEST_CLOSED: {
        frameID = 0;
        break;
    }
    case CHEST_OPENED: {
        frameID = 1;
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_CHEST, frameID * 48, 0, 48, 32);
}

void chest_on_open(g_s *g, obj_s *o)
{
    if (o->state == CHEST_OPENED) return;

    o->flags &= ~OBJ_FLAG_HERO_JUMPABLE;
    o->state = CHEST_OPENED;
}