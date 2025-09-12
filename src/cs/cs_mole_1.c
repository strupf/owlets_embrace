// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_mole_1_update(g_s *g, cs_s *cs, inp_s inp);
void cs_mole_1_on_trigger(g_s *g, cs_s *cs, i32 trigger);

void cs_mole_1_enter(g_s *g)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->on_update  = cs_mole_1_update;
    cs->on_trigger = cs_mole_1_on_trigger;
    g->flags |= GAME_FLAG_BLOCK_PLAYER_INPUT;
}

void cs_mole_1_update(g_s *g, cs_s *cs, inp_s inp)
{
    switch (cs->phase) {
    default: break;
    case 0: {
        if (!cs_wait_and_pause_for_owl_idle(g)) break;

        cs->phase++;
        cs->tick = 0;

        obj_s *ohero = obj_get_owl(g);
        cs->p_owl    = puppet_owl_put(g, ohero);

        for (obj_each(g, o)) {
            if (!(o->ID == OBJID_MISC && o->state == 50)) continue;

            break;
        }
        break;
    }
    }
}

void cs_mole_1_on_trigger(g_s *g, cs_s *cs, i32 trigger)
{
    switch (cs->phase) {
    default: break;
    }
}