// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    int count;
    int count_og;
    int triggers;
    int trigger_enable;
    int trigger_disable;
    int trigger_on_disable;
} clockpulse_s;

void clockpulse_load(game_s *g, map_obj_s *mo)
{
    obj_s *o                = obj_create(g);
    o->ID                   = OBJ_ID_CLOCKPULSE;
    clockpulse_s *cp        = (clockpulse_s *)o->mem;
    int           period_ms = map_obj_i32(mo, "Period");
    o->state                = map_obj_bool(mo, "enabled");
    o->subtimer             = max_i(ticks_from_ms(period_ms), 1);
    cp->count_og            = map_obj_i32(mo, "count");
    cp->triggers            = map_obj_i32(mo, "triggers");
    cp->trigger_enable      = map_obj_i32(mo, "trigger_enable");
    cp->trigger_disable     = map_obj_i32(mo, "trigger_disable");
    cp->trigger_on_disable  = map_obj_i32(mo, "trigger_on_disable");
}

static void clockpulse_disable(game_s *g, obj_s *o)
{
    o->state         = 0;
    clockpulse_s *cp = (clockpulse_s *)o->mem;
    if (cp->trigger_on_disable) {
        game_on_trigger(g, cp->trigger_on_disable);
    }
}

void clockpulse_on_trigger(game_s *g, obj_s *o, int trigger)
{
    clockpulse_s *cp = (clockpulse_s *)o->mem;
    switch (o->state) {
    case 0:
        if (trigger == cp->trigger_enable) {
            o->state  = 1;
            o->timer  = 0;
            cp->count = 0;
        }
        break;
    case 1:
        if (trigger == cp->trigger_disable) {
            clockpulse_disable(g, o);
        }
        break;
    }
}

void clockpulse_on_update(game_s *g, obj_s *o)
{
    if (o->state == 0) return;

    o->timer++;
    if (o->timer < o->subtimer) return;

    clockpulse_s *cp = (clockpulse_s *)o->mem;
    o->timer         = 0;
    game_on_trigger(g, cp->triggers);
    if (cp->count_og) {
        cp->count++;
        if (cp->count_og <= cp->count) {
            clockpulse_disable(g, o);
        }
    }
}