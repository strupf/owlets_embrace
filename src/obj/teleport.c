// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32 tx;
    i32 ty;
} teleport_s;

void teleport_on_interact(g_s *g, obj_s *o)
{
    teleport_s *t   = (teleport_s *)o->mem;
    v2_i32      pos = {t->tx, t->ty};
    maptransition_teleport(g, o->filename, pos);
}

void teleport_load(g_s *g, map_obj_s *mo)
{
    obj_s *o       = obj_create(g);
    o->ID          = OBJ_ID_TELEPORT;
    o->flags       = OBJ_FLAG_INTERACTABLE;
    o->on_interact = teleport_on_interact;
    o->pos.x       = mo->x;
    o->pos.y       = mo->y;
    o->w           = mo->w;
    o->h           = mo->h;
    teleport_s *t  = (teleport_s *)o->mem;
    map_obj_strs(mo, "Target_Map", o->filename);
    t->tx = map_obj_i32(mo, "Target_X");
    t->ty = map_obj_i32(mo, "Target_Y");
}
