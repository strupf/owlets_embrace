// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    u32 hash;
    i32 tx;
    i32 ty;
} teleport_s;

void teleport_on_interact(g_s *g, obj_s *o)
{
    teleport_s *t   = (teleport_s *)o->mem;
    v2_i32      pos = {t->tx, t->ty};
    maptransition_teleport(g, t->hash, pos);
}

void teleport_load(g_s *g, map_obj_s *mo)
{
    obj_s *o                = obj_create(g);
    o->ID                   = OBJID_TELEPORT;
    o->on_interact          = teleport_on_interact;
    o->flags                = OBJ_FLAG_INTERACTABLE;
    o->pos.x                = mo->x;
    o->pos.y                = mo->y;
    o->w                    = mo->w;
    o->h                    = mo->h;
    teleport_s *t           = (teleport_s *)o->mem;
    u8          mapname[16] = {0};
    map_obj_strs(mo, "Target_Map", mapname);
    t->hash = wad_hash(mapname);
    t->tx   = map_obj_i32(mo, "Target_X");
    t->ty   = map_obj_i32(mo, "Target_Y");
}
