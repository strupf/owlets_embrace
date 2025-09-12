// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

// waking up after file select

void cs_on_save_update(g_s *g, cs_s *cs, inp_s inp);

void cs_on_save_enter(g_s *g)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->on_update = cs_on_save_update;
    g->flags |= GAME_FLAG_BLOCK_PLAYER_INPUT;
    obj_s *owl = obj_get_owl(g);
    cs->p_owl  = puppet_owl_put(g, owl);
    puppet_set_anim(cs->p_owl, PUPPET_OWL_ANIMID_IDLE, 0);

    obj_s *comp = obj_get_comp(g);
    if (comp) {
        cs->p_comp = puppet_companion_put(g, comp);
        puppet_set_anim(cs->p_comp, PUPPET_COMPANION_ANIMID_FLY, 0);
    }
}

void cs_on_save_update(g_s *g, cs_s *cs, inp_s inp)
{
    switch (cs->phase) {
    case 0: {
        if (cs->tick < 10) break;

        g->flags &= ~GAME_FLAG_BLOCK_PLAYER_INPUT;
        obj_s *owl = obj_get_owl(g);
        puppet_owl_replace_and_del(g, owl, cs->p_owl);
        if (cs->p_comp) {
            obj_s *comp = obj_get_comp(g);
            puppet_companion_replace_and_del(g, comp, cs->p_comp);
        }
        cs_reset(g);
        break;
    }
    }
}