// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    b32 once;
    i32 triggers;
    i32 trigger_enable;
    i32 trigger_disable;
    i32 trigger_on_disable;
} clockpulse_s;

void clockpulse_on_update(g_s *g, obj_s *o);
void clockpulse_on_trigger(g_s *g, obj_s *o, i32 trigger);

void clockpulse_load(g_s *g, map_obj_s *mo)
{
    obj_s        *o        = obj_create(g);
    clockpulse_s *cp       = (clockpulse_s *)o->mem;
    o->UUID                = mo->UUID;
    o->ID                  = OBJID_CLOCKPULSE;
    o->on_trigger          = clockpulse_on_trigger;
    o->subtimer            = map_obj_i32(mo, "period");
    cp->triggers           = map_obj_i32(mo, "triggers");
    cp->trigger_enable     = map_obj_i32(mo, "trigger_enable");
    cp->trigger_disable    = map_obj_i32(mo, "trigger_disable");
    cp->trigger_on_disable = map_obj_i32(mo, "trigger_on_disable");
    cp->once               = map_obj_bool(mo, "once");

    if (map_obj_bool(mo, "enabled")) {
        o->state     = 1;
        o->on_update = clockpulse_on_update;
    }
}

void clockpulse_on_update(g_s *g, obj_s *o)
{
    clockpulse_s *cp = (clockpulse_s *)o->mem;
    o->timer++;

    if (o->subtimer <= o->timer) {
        o->timer = 0;
        game_on_trigger(g, cp->triggers);

        if (cp->once) {
            o->state     = 0;
            o->on_update = 0;
            game_on_trigger(g, cp->trigger_on_disable);
        }
    }
}

void clockpulse_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    clockpulse_s *cp = (clockpulse_s *)o->mem;

    switch (o->state) {
    case 0:
        if (trigger == cp->trigger_enable) {
            o->state     = 1;
            o->timer     = 0;
            o->on_update = clockpulse_on_update;
        }
        break;
    case 1:
        if (trigger == cp->trigger_disable) {
            o->state     = 0;
            o->on_update = 0;
            game_on_trigger(g, cp->trigger_on_disable);
        }
        break;
    }
}
