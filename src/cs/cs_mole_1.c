// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_mole_1_update(g_s *g, cs_s *cs);
void cs_mole_1_on_trigger(g_s *g, cs_s *cs, i32 trigger);

void cs_mole_1_enter(g_s *g)
{
    cs_s *cs = &g->cuts;
    cs_reset(g);
    cs->on_update         = cs_mole_1_update;
    cs->on_trigger        = cs_mole_1_on_trigger;
    g->block_hero_control = 1;
}

void cs_mole_1_update(g_s *g, cs_s *cs)
{
    switch (cs->phase) {
    default: break;
    case 0: {
        if (!cs_wait_and_pause_for_hero_idle(g)) break;

        cs->phase++;
        cs->tick = 0;

        obj_s *ohero = obj_get_hero(g);
        cs->p_hero   = puppet_hero_put(g, ohero);

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