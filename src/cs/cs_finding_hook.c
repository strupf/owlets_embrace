// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_finding_hook_update(g_s *g, cs_s *cs);

void cs_finding_hook_enter(g_s *g)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->on_update        = cs_finding_hook_update;
    g->block_owl_control = 1;
    save_event_register(g, SAVE_EV_CS_HOOK_FOUND);
}

void cs_finding_hook_update(g_s *g, cs_s *cs)
{
    cs->tick++;

    switch (cs->phase) {
    case 0:
        if (cs_wait_and_pause_for_owl_idle(g)) {
            obj_s *ocomp = obj_get_comp(g);
            if (ocomp) {
                cs->p_comp = puppet_companion_put(g, ocomp);
            }

            cs->phase++;
            cs->tick  = 0;
            cs->p_owl = puppet_owl_put(g, obj_get_owl(g));
            puppet_set_anim(cs->p_owl, PUPPET_OWL_ANIMID_IDLE, -1);
        }
        break;
    case 1:
        if (50 <= cs->tick) {
            if (cs->p_comp) {
                puppet_companion_replace_and_del(g, obj_get_comp(g), cs->p_comp);
            }
            puppet_owl_replace_and_del(g, obj_get_owl(g), cs->p_owl);

#if 0
            hero_add_upgrade(g, HERO_UPGRADE_HOOK);
#endif
            cs_reset(g);
            g->block_owl_control = 0;
        }
        break;
    }
}
