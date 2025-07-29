// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

// second intro cutscene of the first demo room

typedef struct {
    obj_s *puppet_comp;
    obj_s *puppet_hero;
} cs_demo_2_s;

void cs_demo_2_update(g_s *g, cs_s *cs);
void cs_demo_2_on_trigger(g_s *g, cs_s *cs, i32 trigger);
void cs_demo_2_cb_comp(g_s *g, obj_s *o, void *ctx);

void cs_demo_2_enter(g_s *g)
{
    cs_s        *cs = &g->cs;
    cs_demo_2_s *dm = (cs_demo_2_s *)cs->mem;
    cs_reset(g);
    cs->on_update        = cs_demo_2_update;
    cs->on_trigger       = cs_demo_2_on_trigger;
    g->block_owl_control = 1;
    dm->puppet_comp      = obj_find_ID(g, OBJID_PUPPET_COMPANION, 0);

    save_event_register(g, SAVE_EV_INTRO_PLAYED);
}

void cs_demo_2_update(g_s *g, cs_s *cs)
{
    cs_demo_2_s *dm = (cs_demo_2_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    case 0: {
        if (!cs_wait_and_pause_for_owl_idle(g)) break;

        cs->phase++;
        cs->tick = 0;

        obj_s *owl      = obj_get_owl(g);
        dm->puppet_hero = puppet_owl_put(g, owl);
        dialog_open_wad(g, "D_DEMO2_0");
        puppet_set_anim(dm->puppet_hero, PUPPET_OWL_ANIMID_IDLE, 0);
        puppet_move_ext(dm->puppet_comp, (v2_i32){0, -30}, 40, 0, 1, 0, 0);
        break;
    }
    case 4:
        if (30 <= cs->tick) {
            cs->phase++;
            puppet_move_ext(dm->puppet_comp, (v2_i32){0, -150}, 40, ease_in_out_quad, 1, cs_demo_2_cb_comp, cs);
        }
        break;
    }
}

void cs_demo_2_cb_comp(g_s *g, obj_s *o, void *ctx)
{
    cs_s        *cs = (cs_s *)ctx;
    cs_demo_2_s *dm = (cs_demo_2_s *)cs->mem;

    switch (cs->phase) {
    case 2:
        cs->phase++;
        cs->tick = 0;
        dialog_open_wad(g, "D_DEMO2_1");
        g->dialog.script_input = 1;
        snd_play(SNDID_STOMP_LAND, 0.5f, 1.f);
        puppet_set_anim(dm->puppet_comp, PUPPET_COMPANION_ANIMID_BUMP_ONCE, +1);
        puppet_move_ext(dm->puppet_comp, (v2_i32){-30, 0}, 40, 0, 1, cs_demo_2_cb_comp, cs);
        break;
    case 5:
        cs->phase++;
        cs->tick = 0;
        puppet_move_ext(dm->puppet_comp, (v2_i32){200, 0}, 35, ease_in_quad, 1, cs_demo_2_cb_comp, cs);
        break;
    case 6: {
        // leave
        g->block_owl_control = 0;
        obj_s *owl           = obj_get_owl(g);
        puppet_owl_replace_and_del(g, owl, dm->puppet_hero);
        obj_delete(g, dm->puppet_comp);
        cs_reset(g);
        break;
    }
    }
}

void cs_demo_2_on_trigger(g_s *g, cs_s *cs, i32 trigger)
{
    cs_demo_2_s *dm = (cs_demo_2_s *)cs->mem;

    switch (cs->phase) {
    case 1:
        if (trigger == TRIGGER_DIALOG_END) {
            cs->phase++;
            cs->tick = 0;
            for (obj_each_objID(g, i, OBJID_MISC)) {
                if (i->state != 2) continue;
                v2_i32 pc = obj_pos_center(i);
                pc.x -= 8;
                puppet_move_ext(dm->puppet_comp, pc, 40, ease_in_quad, 0, cs_demo_2_cb_comp, cs);
                break;
            }
            break;
        }
    case 3:
        if (trigger == TRIGGER_DIALOG_END) {
            cs->phase++;
            cs->tick = 0;
        }
        break;
    }
}