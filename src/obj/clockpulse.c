// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *clockpulse_create(game_s *g)
{
    obj_s *o    = obj_create(g);
    o->ID       = OBJ_ID_CLOCKPULSE;
    o->trigger  = 1;
    o->subtimer = 30;
    o->state    = 1;
    return o;
}

void clockpulse_on_update(game_s *g, obj_s *o)
{
    if (o->state == 0) return;

    o->timer--;
    if (o->timer <= 0) {
        o->timer = o->subtimer;
        game_on_trigger(g, o->trigger);
    }
}