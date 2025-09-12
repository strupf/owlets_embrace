// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// waking up after file select

#include "game.h"

void cs_on_load_update(g_s *g, cs_s *cs, inp_s inp);
void cs_on_load_title_wakeup(g_s *g);

void cs_on_load_enter(g_s *g)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->on_update = cs_on_load_update;
    g->flags |= GAME_FLAG_BLOCK_PLAYER_INPUT;
    obj_s *owl = obj_get_owl(g);
    cs->p_owl  = puppet_owl_put(g, owl);
    puppet_set_anim(cs->p_owl, PUPPET_OWL_ANIMID_SLEEP, 0);

    obj_s *comp = obj_get_comp(g);
    if (comp) {
        cs->p_comp = puppet_companion_put(g, comp);
        puppet_set_anim(cs->p_comp, PUPPET_COMPANION_ANIMID_SLEEP, 0);

        if (cs->p_owl->pos.x < cs->p_comp->pos.x) {
            cs->p_owl->facing  = +1;
            cs->p_comp->facing = -1;
        } else {
            cs->p_owl->facing  = -1;
            cs->p_comp->facing = +1;
        }
    }
}

void cs_on_load_title_wakeup(g_s *g)
{
    cs_s *cs  = &g->cs;
    cs->phase = 1;
    cs->tick  = 0;
}

void cs_on_load_update(g_s *g, cs_s *cs, inp_s inp)
{
    // any button just pressed
    bool32 btn_any  = inps_btn(inp, INP_A) || inps_btn(inp, INP_B) || inps_x(inp) || inps_y(inp);
    bool32 btn_anyp = inps_btnp(inp, INP_A) || inps_btnp(inp, INP_B) || inps_xp(inp) || inps_yp(inp);

    switch (cs->phase) {
    case 0: break; // blocked until trigger
    case 1: {
        if (btn_any && !btn_anyp) {
            cs->phase++;
            cs->tick = 0;
            puppet_set_anim(cs->p_owl, PUPPET_OWL_ANIMID_SLEEP_WAKEUP, 0);
        }
        break;
    }
    case 2: {
        cs->counter1++; // animation counter owl wakeup

        if (cs->tick == 20) {
            cs->phase++;
            cs->tick = 0;
            if (cs->p_comp) {
                puppet_set_anim(cs->p_comp, PUPPET_COMPANION_ANIMID_WAKEUP, 0);
            }
        }
        break;
    }
    case 3: {
        cs->counter1++; // animation counter owl wakeup

        if (btn_any) {
            cs->counter0 = 1;
        }
        if (cs->counter0) {
            // speed up
            cs->counter1++; // animation counter owl wakeup
            puppet_on_animate(g, cs->p_owl);
        }

        if (ani_len(ANIID_WAKEUP) <= cs->counter1 + 1) {
            puppet_set_anim(cs->p_owl, PUPPET_OWL_ANIMID_IDLE, 0);
            cs->phase++;
            cs->tick = 0;
            if (cs->p_comp) {
                obj_s *comp = obj_get_comp(g);
                puppet_companion_replace_and_del(g, comp, cs->p_comp);
                cs->p_comp = 0;
            }
        }

        break;
    }
    case 4: {
        if (cs->tick < 1) break;

        g->flags &= ~GAME_FLAG_BLOCK_PLAYER_INPUT;
        obj_s *owl = obj_get_owl(g);
        puppet_owl_replace_and_del(g, owl, cs->p_owl);
        cs_reset(g);
        break;
    }
    }
}