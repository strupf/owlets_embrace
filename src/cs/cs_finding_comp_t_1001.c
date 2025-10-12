// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_finding_comp_update(g_s *g, cs_s *cs, inp_s inp);

void cs_finding_comp_enter(g_s *g)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->on_update = cs_finding_comp_update;
    cs->p_comp    = obj_find_ID(g, OBJID_PUPPET_COMPANION, 0);
    g->flags |= GAME_FLAG_BLOCK_PLAYER_INPUT;
    puppet_set_anim(cs->p_comp, PUPPET_COMPANION_ANIMID_FLY, -1);
    saveID_put(g, SAVEID_COMPANION_FOUND);
}

void cs_finding_comp_update(g_s *g, cs_s *cs, inp_s inp)
{
    cs->tick++;
    switch (cs->phase) {
    case 0:
        if (cs_wait_and_pause_for_owl_idle(g)) {
            cs->phase++;
            cs->tick  = 0;
            cs->p_owl = puppet_owl_put(g, obj_get_owl(g));
            puppet_set_anim(cs->p_owl, PUPPET_OWL_ANIMID_IDLE, -1);
        }
        break;
    case 1:
        if (50 <= cs->tick) {
            obj_s *op = companion_create(g);
            puppet_companion_replace_and_del(g, op, cs->p_comp);
            obj_s *owl = obj_get_owl(g);
            puppet_owl_replace_and_del(g, owl, cs->p_owl);
            owl_upgrade_add(owl, OWL_UPGRADE_COMPANION);
            cs_reset(g);
            g->flags &= ~GAME_FLAG_BLOCK_PLAYER_INPUT;
        }
        break;
    }
}
