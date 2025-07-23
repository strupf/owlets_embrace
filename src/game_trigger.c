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
        boss_plant_wake_up(g);
        break;
    case 9000:
        cs_demo_1_enter(g);
        break;
    case 9001:
        cs_demo_2_enter(g);
        break;
    case 9005:
        cs_demo_3_enter(g);
        break;
    case TRIGGER_CS_UPGRADE:
        cs_powerup_enter(g);
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
    case 90:
        cs_mole_1_enter(g);
        break;
    }

    if (g->cuts.on_trigger) {
        g->cuts.on_trigger(g, &g->cuts, trigger);
    }

    for (obj_each(g, o)) {
        if (o->on_trigger) {
            o->on_trigger(g, o, trigger);
        }
    }
}