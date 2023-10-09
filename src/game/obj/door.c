// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"

enum {
        DOOR_STATE_CLOSED,
        DOOR_STATE_MOVING,
        DOOR_STATE_OPEN,
};

enum {
        DOOR_TYPE_PHYSICAL, // only a physical barrier
        DOOR_TYPE_NEW_MAP,
};

typedef struct {
        obj_s o;
        int   state;
        int   open_ticks;
        int   ticks;
} door_s;

static void door_think(game_s *g, obj_s *o);
static void door_trigger(game_s *g, obj_s *o, int triggerID);
static void door_interact(game_s *g, obj_s *o);
static void door_think_new_map_opening(game_s *g, obj_s *o);

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
        o->ID        = OBJ_ID_DOOR;

        door_s *d = (door_s *)o;
        d->state  = DOOR_STATE_CLOSED;
        return o;
}

obj_s *door_create_new_map(game_s *g)
{
        obj_s *o = obj_create(g);
        obj_apply_flags(g, o, OBJ_FLAG_INTERACT);
        o->pos.x      = 16 * 17 + 5 * 16;
        o->pos.y      = 192 - 130 - 64;
        o->w          = 16;
        o->h          = 64;
        o->oninteract = door_interact;
        o->state      = DOOR_STATE_CLOSED;
        o->ID         = OBJ_ID_DOOR;

        door_s *d = (door_s *)o;
        d->state  = DOOR_STATE_CLOSED;

        return o;
}

obj_s *door_create_physical(game_s *g)
{
        obj_s *o = obj_create(g);
        obj_apply_flags(g, o, OBJ_FLAG_SOLID);
        o->pos.x     = 16 * 17 + 5 * 16;
        o->pos.y     = 192 - 130 - 64;
        o->w         = 16;
        o->h         = 64 + 16;
        o->ontrigger = door_trigger;
        o->state     = DOOR_STATE_CLOSED;
        o->ID        = OBJ_ID_DOOR;

        door_s *d = (door_s *)o;
        d->state  = DOOR_STATE_CLOSED;

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

static void door_think_new_map_opening(game_s *g, obj_s *o)
{
        if (--o->timer <= 0) {
                o->state = DOOR_STATE_OPEN;
                transition_start(g, o->filename, (v2_i32){100, 100}, 0);
                return;
        }
        o->tomove.y = -1;
}

static void door_interact(game_s *g, obj_s *o)
{
        char filename[64] = {0};
        os_strcat(filename, "assets/map/");
        os_strcat(filename, o->filename);
        transition_start(g, filename, (v2_i32){100, 100}, 0);
}