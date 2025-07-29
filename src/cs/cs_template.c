// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_template_update(g_s *g, cs_s *cs);
void cs_template_on_trigger(g_s *g, cs_s *cs, i32 trigger);
void cs_template_cb_comp(g_s *g, obj_s *o, void *ctx);

void cs_template_enter(g_s *g)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->on_update        = cs_template_update;
    cs->on_trigger       = cs_template_on_trigger;
    g->block_owl_control = 1;
    cs->p_comp           = obj_find_ID(g, OBJID_PUPPET_COMPANION, 0);
}

void cs_template_update(g_s *g, cs_s *cs)
{
    switch (cs->phase) {
    default: break;
    case 0: {
        if (!cs_wait_and_pause_for_owl_idle(g)) break;

        cs->phase++;
        cs->tick = 0;

        obj_s *ohero = obj_get_owl(g);
        cs->p_owl    = puppet_owl_put(g, ohero);
        puppet_set_anim(cs->p_owl, PUPPET_OWL_ANIMID_IDLE, +1);
        puppet_move_ext(cs->p_comp, (v2_i32){-30, -50}, 30, 0, 1, cs_template_cb_comp, cs);
        puppet_set_anim(cs->p_comp, PUPPET_COMPANION_ANIMID_FLY, -1);
        break;
    }
    }
}

void cs_template_cb_comp(g_s *g, obj_s *o, void *ctx)
{
    cs_s *cs = (cs_s *)ctx;

    switch (cs->phase) {
    default: break;
    }
}

void cs_template_on_trigger(g_s *g, cs_s *cs, i32 trigger)
{
    switch (cs->phase) {
    default: break;
    }
}