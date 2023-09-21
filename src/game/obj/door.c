// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"

enum {
        DOOR_STATE_CLOSED,
        DOOR_STATE_MOVING,
        DOOR_STATE_OPEN,
};

static void door_think(game_s *g, obj_s *o);
static void door_trigger(game_s *g, obj_s *o, int triggerID);

obj_s *door_create(game_s *g)
{
        obj_s *o = obj_create(g);
        obj_apply_flags(g, o, OBJ_FLAG_SOLID);
        o->pos.x     = 16 * 17 + 5 * 16;
        o->pos.y     = 192 - 130 - 64;
        o->w         = 16;
        o->h         = 64 + 16;
        o->ontrigger = door_trigger;
        o->state     = DOOR_STATE_CLOSED;
        o->ID        = 4;
        return o;
}

static void door_think(game_s *g, obj_s *o)
{
        if (--o->timer <= 0) {
                o->state = DOOR_STATE_OPEN;
                obj_unset_flags(g, o, OBJ_FLAG_THINK_1);
                return;
        }
        o->tomove.y = -1;
}

static void door_trigger(game_s *g, obj_s *o, int triggerID)
{
        if (o->ID == triggerID) {
                o->ontrigger = NULL;
                obj_set_flags(g, o, OBJ_FLAG_THINK_1);
                o->think_1 = door_think;
                o->timer   = 60;
                o->state   = DOOR_STATE_MOVING;
        }
}