// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void crankswitch_on_update(game_s *g, obj_s *o);
void crankswitch_on_animate(game_s *g, obj_s *o);

void crankswitch_load(game_s *g, map_obj_s *mo)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJ_ID_CRANKSWITCH;
    o->on_update  = crankswitch_on_update;
    o->on_animate = crankswitch_on_animate;
}

void crankswitch_on_update(game_s *g, obj_s *o)
{
}

void crankswitch_on_animate(game_s *g, obj_s *o)
{
}

void crankswitch_on_turn(game_s *g, obj_s *o, i32 angle_q8)
{
    i32 a0 = o->state;
    i32 a1 = (a0 + angle_q8) & 0xFF;
    i32 da = inp_crank_calc_dt_qx(8, a0, a1);

    for (obj_each(g, it)) {
        switch (it->ID) {
        default: break;
        }
    }
}