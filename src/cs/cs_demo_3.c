// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

// cutscene companion permanently being added to the player

typedef struct {
    obj_s *puppet_comp;
    obj_s *puppet_hero;
} cs_demo_3_s;

void cs_demo_3_update(g_s *g, cs_s *cs);
void cs_demo_3_on_trigger(g_s *g, cs_s *cs, i32 trigger);
void cs_demo_3_cb_comp(g_s *g, obj_s *o, void *ctx);

void cs_demo_3_enter(g_s *g)
{
    cs_s        *cs = &g->cs;
    cs_demo_3_s *dm = (cs_demo_3_s *)cs->mem;
    cs_reset(g);
    cs->on_update        = cs_demo_3_update;
    cs->on_trigger       = cs_demo_3_on_trigger;
    g->block_owl_control = 1;
    dm->puppet_comp      = obj_find_ID(g, OBJID_PUPPET_COMPANION, 0);

    save_event_register(g, SAVE_EV_COMPANION_FOUND);
}

void cs_demo_3_update(g_s *g, cs_s *cs)
{
    cs_demo_3_s *dm = (cs_demo_3_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    case 0: {
        if (!cs_wait_and_pause_for_owl_idle(g)) break;

        cs->phase++;
        cs->tick = 0;

        obj_s *owl      = obj_get_owl(g);
        dm->puppet_hero = puppet_owl_put(g, owl);
        dialog_open_wad(g, "D_DEMO3_0");
        puppet_set_anim(dm->puppet_hero, PUPPET_OWL_ANIMID_IDLE, 0);
        puppet_move_ext(dm->puppet_comp, (v2_i32){0, -30}, 40, 0, 1, 0, 0);
        puppet_set_anim(dm->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, -1);
        break;
    }
    case 2: {
        if (10 <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
            puppet_set_anim(dm->puppet_hero, PUPPET_OWL_ANIMID_HOLD_ARM, 0);
            v2_i32 phero = obj_pos_center(dm->puppet_hero);
            phero.y -= 16;
            phero.x -= dm->puppet_hero->facing * 8;
            puppet_move_ext(dm->puppet_comp, phero, 40, ease_in_out_quad, 0, 0, 0);
        }
        break;
    }
    case 3: {
        if (40 <= cs->tick) {
            // leave
            g->block_owl_control = 0;
            obj_s *owl           = obj_get_owl(g);
            puppet_owl_replace_and_del(g, owl, dm->puppet_hero);
            obj_s *ocomp = companion_create(g);
            puppet_companion_replace_and_del(g, ocomp, dm->puppet_comp);

#if 0
            g->hero.mode = HERO_MODE_COMBAT;
            hero_add_upgrade(g, HERO_UPGRADE_COMPANION);
#endif
            cs_reset(g);
        }
        break;
    }
    }
}

void cs_demo_3_cb_comp(g_s *g, obj_s *o, void *ctx)
{
    cs_s        *cs = (cs_s *)ctx;
    cs_demo_3_s *dm = (cs_demo_3_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    }
}

void cs_demo_3_on_trigger(g_s *g, cs_s *cs, i32 trigger)
{
    cs_demo_3_s *dm = (cs_demo_3_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    case 1: {
        if (trigger == TRIGGER_DIALOG_END) {
            cs->phase++;
            cs->tick = 0;
        }
    }
    }
}