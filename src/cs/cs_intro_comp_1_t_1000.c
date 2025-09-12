// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_intro_comp_1_update(g_s *g, cs_s *cs, inp_s inp);

void cs_intro_comp_1_enter(g_s *g)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->on_update = cs_intro_comp_1_update;
    cs->p_comp    = obj_find_ID(g, OBJID_PUPPET_COMPANION, 0);
    cs->p_comp->pos.y += 4;
    puppet_set_anim(cs->p_comp, PUPPET_COMPANION_ANIMID_FLY, -1);
    save_event_register(g, SAVE_EV_CS_INTRO_COMP_1);
}

void cs_intro_comp_1_update(g_s *g, cs_s *cs, inp_s inp)
{
    if (0) {
    } else if (cs->tick < 60) {
        cs->p_comp->pos.x += 2;
    } else if (cs->tick < 90) {
    } else if (cs->tick < 200) {
        cs->counter0 = min_i32(cs->counter0 + 1, 6);
        puppet_set_anim(cs->p_comp, 0, +1);
        cs->p_comp->pos.x += cs->counter0;
    } else {
        obj_delete(g, cs->p_comp);
        cs_reset(g);
    }
}
