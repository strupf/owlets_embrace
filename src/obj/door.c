// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

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
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_DOOR;
    o->flags = OBJ_FLAG_SOLID |
               OBJ_FLAG_RENDER_AABB;

    o->w = 16;
    o->h = 16;

    return o;
}

void door_on_update(game_s *g, obj_s *o)
{
    door_s *door = (door_s *)o->mem;

    if (o->state == DOOR_STATE_CLOSED || o->state == DOOR_STATE_OPEN) return;

    o->timer++;

    switch (o->state) {
    case DOOR_STATE_OPENING:
        if (o->timer < door->ticks_to_open) break;
        o->state  = DOOR_STATE_OPEN;
        o->tomove = v2_sub(door->pos_slide_open, o->pos);
        return;
    case DOOR_STATE_CLOSING:
        if (o->timer < door->ticks_to_close) break;
        o->state  = DOOR_STATE_CLOSED;
        o->tomove = v2_sub(door->pos_slide_closed, o->pos);
        return;
    }

    switch (door->type) {
    case DOOR_TYPE_SLIDING: {
        v2_i32 p0 = door->pos_slide_closed;
        v2_i32 p1 = door->pos_slide_open;

        if (o->state == DOOR_STATE_CLOSING) {
            SWAP(v2_i32, p0, p1);
        }

        int    ticks = o->state == DOOR_STATE_CLOSING ? door->ticks_to_close : door->ticks_to_open;
        v2_i32 pos   = v2_lerp(p0, p1, o->timer, ticks);
        o->tomove    = v2_sub(pos, o->pos);
    } break;
    }
}

void door_on_trigger(game_s *g, obj_s *obj, int trigger)
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