// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

// first intro cutscene of the first demo room

void cs_demo_1_update(g_s *g, cs_s *cs);
void cs_demo_1_on_trigger(g_s *g, cs_s *cs, i32 trigger);
void cs_demo_1_cb_comp(g_s *g, obj_s *o, void *ctx);

void cs_demo_1_enter(g_s *g)
{
    cs_s *cs = &g->cuts;
    cs_reset(g);
    cs->on_update         = cs_demo_1_update;
    cs->on_trigger        = cs_demo_1_on_trigger;
    g->block_hero_control = 1;
    cs->p_comp            = obj_find_ID(g, OBJID_PUPPET_COMPANION, 0);
}

void cs_demo_1_update(g_s *g, cs_s *cs)
{
    switch (cs->phase) {
    default: break;
    case 0: {
        if (!cs_wait_and_pause_for_hero_idle(g)) break;

        cs->phase++;
        cs->tick = 0;

        obj_s *ohero = obj_get_hero(g);
        cs->p_hero   = puppet_hero_put(g, ohero);
        puppet_set_anim(cs->p_hero, PUPPET_HERO_ANIMID_IDLE, +1);
        puppet_move_ext(cs->p_comp, (v2_i32){-30, -50}, 30, 0, 1, cs_demo_1_cb_comp, cs);
        puppet_set_anim(cs->p_comp, PUPPET_COMPANION_ANIMID_FLY, -1);
        break;
    }
    case 3:
        if (30 <= cs->tick) {
            cs->phase++;
            cs->tick = 0;
            dialog_open_wad(g, "D_DEMO1_1");
            // g->dialog.pos = DIALOG_POS_TOP;
            puppet_set_anim(cs->p_comp, 0, -1);
        }
        break;
    }
}

void cs_demo_1_cb_comp(g_s *g, obj_s *o, void *ctx)
{
    cs_s *cs = (cs_s *)ctx;

    switch (cs->phase) {
    case 1:
        dialog_open_wad(g, "D_DEMO1_0");
        // g->dialog.pos = DIALOG_POS_TOP;
        puppet_set_anim(cs->p_comp, PUPPET_COMPANION_ANIMID_NOD_ONCE, 0);
        break;
    case 2:
        cs->phase++;
        cs->tick = 0;
        break;
    case 5:
        dialog_open_wad(g, "D_DEMO1_2");
        g->dialog.pos = DIALOG_POS_TOP;
        puppet_set_anim(cs->p_comp, PUPPET_COMPANION_ANIMID_NOD_ONCE, 0);
        break;
    case 6:
        cs->phase++;
        for (obj_each_objID(g, i, OBJID_MISC)) {
            if (i->state != 1) continue;

            v2_i32 pc = obj_pos_center(i);
            puppet_move_ext(cs->p_comp, pc, 60, ease_in_quad, 0, cs_demo_1_cb_comp, cs);
            break;
        }
        break;
    case 7: {
        // leave
        cs->p_comp->facing    = -1;
        g->block_hero_control = 0;
        obj_s *ohero          = obj_get_hero(g);
        puppet_hero_replace_and_del(g, ohero, cs->p_hero);
        cs_reset(g);
        break;
    }
    }
}

void cs_demo_1_on_trigger(g_s *g, cs_s *cs, i32 trigger)
{
    switch (cs->phase) {
    case 1:
        if (trigger == TRIGGER_DIALOG_END) {
            cs->phase++;
            cs->tick = 0;

            puppet_move_ext(cs->p_comp, (v2_i32){90, -60}, 170, 0, 1, cs_demo_1_cb_comp, cs);
            puppet_set_anim(cs->p_comp, PUPPET_COMPANION_ANIMID_FLY, +1);
        }
        break;
    case 4:
        if (trigger == TRIGGER_DIALOG_END) {
            cs->phase++;
            cs->tick = 0;

            puppet_move_ext(cs->p_comp, (v2_i32){cs->p_hero->pos.x + 40, cs->p_hero->pos.y - 24}, 50, ease_in_out_quad, 0, cs_demo_1_cb_comp, cs);
            puppet_set_anim(cs->p_comp, 0, -1);
        }
        break;
    case 5:
        if (trigger == TRIGGER_DIALOG_END) {
            cs->phase++;
            puppet_set_anim(cs->p_comp, 0, +1);
            puppet_move_ext(cs->p_comp, (v2_i32){20, -64}, 35, ease_in_out_quad, 1, cs_demo_1_cb_comp, cs);
        }
        break;
    }
}