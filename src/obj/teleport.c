// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    u8  trg_map_name[MAP_WAD_NAME_LEN];
    i32 trg_x;
    i32 trg_y;
} teleport_s;

void teleport_on_interact(g_s *g, obj_s *o);
void teleport_on_animate(g_s *g, obj_s *o);
void teleport_on_trigger(g_s *g, obj_s *o, i32 trigger);

void teleport_load(g_s *g, map_obj_s *mo)
{
    i32 teleporterID = map_obj_i32(mo, "teleporterID");

    switch (teleporterID) {
    case 0: break;
    }

    obj_s      *o  = obj_create(g);
    teleport_s *t  = (teleport_s *)o->mem;
    o->ID          = OBJID_TELEPORT;
    o->subID       = teleporterID;
    o->on_interact = teleport_on_interact;
    o->on_animate  = teleport_on_animate;
    o->on_trigger  = teleport_on_trigger;
    o->flags       = OBJ_FLAG_INTERACTABLE;
    o->pos.x       = mo->x;
    o->pos.y       = mo->y;
    o->w           = mo->w;
    o->h           = mo->h;
    map_obj_strs(mo, "trg_map_name", t->trg_map_name);
    t->trg_x = map_obj_i32(mo, "trg_x");
    t->trg_y = map_obj_i32(mo, "trg_y");
}

void teleport_on_interact(g_s *g, obj_s *o)
{
    teleport_s *t   = (teleport_s *)o->mem;
    v2_i32      pos = {t->trg_x, t->trg_y};
    cs_maptransition_teleport(g, t->trg_map_name, pos);
}

void teleport_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];

    switch (o->subID) {
    case 0: break;
    }
}

void teleport_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
}