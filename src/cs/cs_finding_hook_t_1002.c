// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_finding_hook_update(g_s *g, cs_s *cs, inp_s inp);

void cs_finding_hook_enter(g_s *g)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->on_update = cs_finding_hook_update;
    g->flags |= GAME_FLAG_BLOCK_PLAYER_INPUT;
    saveID_put(g, SAVEID_CS_HOOK_FOUND);
}

void cs_finding_hook_update(g_s *g, cs_s *cs, inp_s inp)
{
    cs->tick++;

    switch (cs->phase) {
    case 0:
        if (cs_wait_and_pause_for_owl_idle(g)) {
            cs->phase++;
            cs->tick  = 0;
            cs->p_owl = puppet_owl_put(g, obj_get_owl(g));
            puppet_set_anim(cs->p_owl, PUPPET_OWL_ANIMID_IDLE, -1);

            obj_s *ocomp = obj_get_comp(g);
            if (ocomp) {
                cs->p_comp = puppet_companion_put(g, ocomp);
            }
        }
        break;
    case 1:
        if (50 <= cs->tick) {
            if (cs->p_comp) {
                puppet_companion_replace_and_del(g, obj_get_comp(g), cs->p_comp);
            }
            obj_s *owl = obj_get_owl(g);
            puppet_owl_replace_and_del(g, owl, cs->p_owl);
            owl_upgrade_add(owl, OWL_UPGRADE_HOOK);
            cs_reset(g);
            g->flags &= ~GAME_FLAG_BLOCK_PLAYER_INPUT;
        }
        break;
    }
}
