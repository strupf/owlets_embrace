// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void game_on_trigger(g_s *g, i32 trigger)
{
    if (!trigger) return;
    pltf_log("trigger %i\n", trigger);

    switch (trigger) {
    case TRIGGER_BOSS_PLANT:
        // boss_plant_wake_up(g);
        // bossplant_awake(g);
        break;
    case TRIGGER_CS_INTRO_COMP_1:
        cs_intro_comp_1_enter(g);
        break;
    case TRIGGER_CS_FINDING_COMP:
        cs_finding_comp_enter(g);
        break;
    case TRIGGER_CS_FINDING_HOOK:
        cs_finding_hook_enter(g);
        break;
    }

    if (g->cs.on_trigger) {
        g->cs.on_trigger(g, &g->cs, trigger);
    }

    for (obj_each(g, o)) {
        if (o->on_trigger) {
            o->on_trigger(g, o, trigger);
        }
    }
}