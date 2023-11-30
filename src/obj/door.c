// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    DOOR_CLOSED,
    DOOR_OPEN,
    DOOR_OPENING,
    DOOR_CLOSING,
};

enum {
    DOOR_TYPE_SLIDING,
};

enum {
    DOOR_ACTIVATE_TRIGGER,
    DOOR_ACTIVATE_KEY,
    DOOR_ACTIVATE_INTERACT,
};

typedef struct {
    obj_s o;

    int state;
    int type;
    int activate;
} door_s;

void door_update(game_s *g, obj_s *o)
{
    door_s *door = (door_s *)o;
}

void door_activate(door_s *door)
{
    switch (door->state) {
    case DOOR_CLOSED: {
        door->state = DOOR_OPENING;
    } break;
    case DOOR_OPEN: {
        door->state = DOOR_CLOSING;
    } break;
    }
}

void door_trigger(game_s *g, obj_s *o, int trigger)
{
    door_s *door = (door_s *)o;
    if (o->trigger != trigger) return;

    door_activate(door);
}