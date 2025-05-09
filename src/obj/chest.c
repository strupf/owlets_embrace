// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    CHEST_CLOSED,
    CHEST_OPENED,
};

typedef struct {
    i32 saveID;
} chest_s;

void chest_on_update(g_s *g, obj_s *o);
void chest_on_animate(g_s *g, obj_s *o);

void chest_load(g_s *g, map_obj_s *mo)
{
    obj_s   *o    = obj_create(g);
    chest_s *c    = (chest_s *)o->mem;
    o->ID         = OBJID_CHEST;
    o->on_update  = chest_on_update;
    o->on_animate = chest_on_animate;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->w          = mo->w;
    o->h          = mo->h;
    o->n_sprites  = 1;
    i32 saveID    = map_obj_i32(mo, "saveID");
    c->saveID     = saveID;
    if (save_event_exists(g, saveID)) {
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

    chest_s *c = (chest_s *)o->mem;
    o->flags &= ~OBJ_FLAG_HERO_JUMPABLE;
    o->state = CHEST_OPENED;
    save_event_register(g, c->saveID);
    v2_i32 p = obj_pos_center(o);
    p.y -= 6;
    particle_emit_ID(g, PARTICLE_EMIT_ID_CHEST, p);
    g->freeze_tick = 2;
}