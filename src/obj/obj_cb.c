// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    void *arg;
    void (*on_upd)(g_s *g, void *arg);
    void (*on_trigger)(g_s *g, i32 trigger, void *arg);
} obj_cb_s;

void obj_cb_on_update(g_s *g, obj_s *o);
void obj_cb_on_trigger(g_s *g, obj_s *o, i32 trigger);

obj_s *obj_cb_create(g_s *g, void *arg)
{
    obj_s *o    = obj_create(g);
    o->ID       = OBJID_OBJ_CB;
    obj_cb_s *c = (obj_cb_s *)o->mem;
    c->arg      = arg;
    return o;
}

void obj_cb_set_on_update(obj_s *o, void (*on_upd)(g_s *g, void *arg))
{
    obj_cb_s *c = (obj_cb_s *)o->mem;
    c->on_upd   = on_upd;
    if (on_upd) {
        o->on_update = obj_cb_on_update;
    } else {
        o->on_update = 0;
    }
}

void obj_cb_set_on_trigger(obj_s *o, void (*on_trigger)(g_s *g, i32 trigger, void *arg))
{
    obj_cb_s *c   = (obj_cb_s *)o->mem;
    c->on_trigger = on_trigger;
    if (on_trigger) {
        o->on_trigger = obj_cb_on_trigger;
    } else {
        o->on_trigger = 0;
    }
}

void obj_cb_on_update(g_s *g, obj_s *o)
{
    obj_cb_s *c = (obj_cb_s *)o->mem;
    if (c->on_upd) {
        c->on_upd(g, c->arg);
    }
}

void obj_cb_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    obj_cb_s *c = (obj_cb_s *)o->mem;
    if (c->on_trigger) {
        c->on_trigger(g, trigger, c->arg);
    }
}