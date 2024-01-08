// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#if 1
enum {
    DOOR_STATE_CLOSED,
    DOOR_STATE_OPEN,
    DOOR_STATE_OPENING,
    DOOR_STATE_CLOSING,
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

    int    state;
    int    type;
    bool32 can_open;
    bool32 can_close;

    int tick;
    int ticks_to_open;
    int ticks_to_close;

    v2_i32 pos_slide_open;
    v2_i32 pos_slide_closed;
} door_s;

obj_s *door_create(game_s *g)
{
    door_s *o = (door_s *)obj_create(g);

    return (obj_s *)o;
}

void door_update(game_s *g, obj_s *obj)
{
    door_s *o = (door_s *)obj;

    if (o->state == DOOR_STATE_CLOSED || o->state == DOOR_STATE_OPEN) return;

    o->tick++;

    switch (o->state) {
    case DOOR_STATE_OPENING:
        if (o->tick < o->ticks_to_open) break;
        o->state    = DOOR_STATE_OPEN;
        obj->tomove = v2_sub(o->pos_slide_open, obj->pos);
        return;
    case DOOR_STATE_CLOSING:
        if (o->tick < o->ticks_to_close) break;
        o->state    = DOOR_STATE_CLOSED;
        obj->tomove = v2_sub(o->pos_slide_closed, obj->pos);
        return;
    }

    switch (o->type) {
    case DOOR_TYPE_SLIDING: {
        v2_i32 p0 = o->pos_slide_closed;
        v2_i32 p1 = o->pos_slide_open;

        if (o->state == DOOR_STATE_CLOSING) {
            SWAP(v2_i32, p0, p1);
        }

        int    ticks = o->state == DOOR_STATE_CLOSING ? o->ticks_to_close : o->ticks_to_open;
        v2_i32 pos   = v2_lerp(p0, p1, o->tick, ticks);
        obj->tomove  = v2_sub(pos, obj->pos);
    } break;
    }
}

void door_trigger(game_s *g, obj_s *obj, int trigger)
{
    door_s *o = (door_s *)obj;

    switch (o->state) {
    case DOOR_STATE_CLOSED: {
        if (!o->can_open) break;

        o->state = DOOR_STATE_OPENING;
        o->tick  = 0;
    } break;
    case DOOR_STATE_OPEN: {
        if (!o->can_close) break;

        o->state = DOOR_STATE_CLOSING;
        o->tick  = 0;
    } break;
    }
}
#endif