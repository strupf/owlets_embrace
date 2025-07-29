// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

// cutscene companion permanently being added to the player

typedef struct {
    obj_s *puppet_comp;
    obj_s *puppet_hero;
} cs_demo_4_s;

void cs_demo_4_update(g_s *g, cs_s *cs);
void cs_demo_4_on_trigger(g_s *g, cs_s *cs, i32 trigger);
void cs_demo_4_cb_comp(g_s *g, obj_s *o, void *ctx);

void cs_demo_4_enter(g_s *g)
{
    cs_s        *cs = &g->cs;
    cs_demo_4_s *dm = (cs_demo_4_s *)cs->mem;
    cs_reset(g);
    cs->on_update        = cs_demo_4_update;
    cs->on_trigger       = cs_demo_4_on_trigger;
    g->block_owl_control = 1;
    dm->puppet_comp      = obj_find_ID(g, OBJID_PUPPET_COMPANION, 0);

    save_event_register(g, SAVE_EV_COMPANION_FOUND);
}

void cs_demo_4_update(g_s *g, cs_s *cs)
{
    cs_demo_4_s *dm = (cs_demo_4_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    }
}

void cs_demo_4_cb_comp(g_s *g, obj_s *o, void *ctx)
{
    cs_s        *cs = (cs_s *)ctx;
    cs_demo_4_s *dm = (cs_demo_4_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    }
}

void cs_demo_4_on_trigger(g_s *g, cs_s *cs, i32 trigger)
{
    cs_demo_4_s *dm = (cs_demo_4_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    case 1: {
    }
    }
}