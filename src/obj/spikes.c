// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define SPIKE_SIZE    4
#define SPIKE_SIZE_DT (16 - SPIKE_SIZE)

enum {
    SPIKES_STATIC,
    SPIKES_DYNAMIC, // can be triggered
};

typedef struct {
    i32 trigger_on;
    i32 trigger_off;
} spikes_s;

void spikes_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_SPIKES;
    o->flags = OBJ_FLAG_RENDER_AABB;
    o->pos.x = mo->x;
    o->pos.y = mo->y;
    o->w     = mo->w;
    o->h     = mo->h;
    if (0) {
    } else if (str_contains(mo->name, "_L")) {
        o->facing = DIRECTION_W;
        o->w -= SPIKE_SIZE_DT;
        o->pos.x += SPIKE_SIZE_DT;
    } else if (str_contains(mo->name, "_R")) {
        o->facing = DIRECTION_E;
        o->w -= SPIKE_SIZE_DT;
    } else if (str_contains(mo->name, "_U")) {
        o->facing = DIRECTION_N;
        o->h -= SPIKE_SIZE_DT;
        o->pos.y += SPIKE_SIZE_DT;
    } else if (str_contains(mo->name, "_D")) {
        o->facing = DIRECTION_S;
        o->h -= SPIKE_SIZE_DT;
    }
    spikes_s *sp    = (spikes_s *)o->mem;
    o->substate     = map_obj_bool(mo, "enabled");
    int t_on        = map_obj_i32(mo, "trigger_on");
    int t_off       = map_obj_i32(mo, "trigger_off");
    o->state        = t_on || t_off ? SPIKES_DYNAMIC : SPIKES_STATIC;
    sp->trigger_off = t_off;
    sp->trigger_on  = t_on;

    if (o->state == SPIKES_STATIC || o->substate) {
        o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
    }
}

void spikes_on_trigger(game_s *g, obj_s *o, int trigger)
{
    if (o->state != SPIKES_DYNAMIC) return;

    spikes_s *sp = (spikes_s *)o->mem;

    switch (o->substate) {
    case 0:
        if (trigger == sp->trigger_on) {
            o->substate = 1;
            o->timer    = 0;
            o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
        }
        break;
    case 1:
        if (trigger == sp->trigger_off) {
            o->substate = 0;
            o->timer    = 0;
            o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
        }
        break;
    }
}

void spikes_on_animate(game_s *g, obj_s *o)
{
    o->timer++;
}

void spikes_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
    switch (o->state) {
    case SPIKES_STATIC:
        break;
    case SPIKES_DYNAMIC:
        break;
    }
}