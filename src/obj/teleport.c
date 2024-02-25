// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    int tx;
    int ty;
} teleport_s;

obj_s *teleport_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_TELEPORT;
    o->flags = OBJ_FLAG_INTERACTABLE;
    return o;
}

void teleport_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = teleport_create(g);

    o->pos.x      = mo->x;
    o->pos.y      = mo->y;
    o->w          = mo->w;
    o->h          = mo->h;
    teleport_s *t = (teleport_s *)o->mem;
    map_obj_strs(mo, "Target_Map", o->filename);
    t->tx = map_obj_i32(mo, "Target_X");
    t->ty = map_obj_i32(mo, "Target_Y");
}

void teleport_on_interact(game_s *g, obj_s *o)
{
    teleport_s *t   = (teleport_s *)o->mem;
    v2_i32      pos = {t->tx, t->ty};
    substate_transition_teleport(g, &g->substate, o->filename, pos);
}