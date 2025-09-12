// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    obj_s *puppet_comp;
    obj_s *puppet_hero;
} cs_explain_hook_s;

void cs_explain_hook_update(g_s *g, cs_s *cs, inp_s inp);
void cs_explain_hook_on_trigger(g_s *g, cs_s *cs, i32 trigger);
void cs_explain_hook_cb_comp(g_s *g, obj_s *o, void *ctx);

void cs_explain_hook_enter(g_s *g)
{
    cs_s              *cs = &g->cs;
    cs_explain_hook_s *dm = (cs_explain_hook_s *)cs->mem;
    cs_reset(g);
    cs->on_update  = cs_explain_hook_update;
    cs->on_trigger = cs_explain_hook_on_trigger;
    g->flags |= GAME_FLAG_BLOCK_PLAYER_INPUT;
}

void cs_explain_hook_update(g_s *g, cs_s *cs, inp_s inp)
{
    cs_explain_hook_s *dm = (cs_explain_hook_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    case 0: {
        if (!cs_wait_and_pause_for_owl_idle(g)) break;

        cs->phase++;
        cs->tick = 0;

        obj_s *owl      = obj_get_owl(g);
        obj_s *ocomp    = obj_get_tagged(g, OBJ_TAG_COMPANION);
        dm->puppet_hero = puppet_owl_put(g, owl);
        puppet_set_anim(dm->puppet_hero, PUPPET_OWL_ANIMID_IDLE, 0);
        dm->puppet_comp = puppet_companion_put(g, ocomp);
        v2_i32 hpos     = obj_pos_center(owl);
        hpos.x += owl->facing * 40;
        hpos.y -= 15;

        i32 cfacing = hpos.x < dm->puppet_comp->pos.x ? -1 : +1;
        puppet_set_anim(dm->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, cfacing);
        puppet_move_ext(dm->puppet_comp, hpos, 30, 0, 0,
                        cs_explain_hook_cb_comp, cs);
        break;
    }
    case 3: {
        if (100 <= cs->tick) {
            cs->phase = 10;
        }
        break;
    }
    case 10: {
        // leave
        g->flags &= ~GAME_FLAG_BLOCK_PLAYER_INPUT;
        obj_s *owl   = obj_get_owl(g);
        obj_s *ocomp = obj_get_tagged(g, OBJ_TAG_COMPANION);
        puppet_owl_replace_and_del(g, owl, dm->puppet_hero);
        puppet_companion_replace_and_del(g, ocomp, dm->puppet_comp);
        cs_reset(g);
        break;
    }
    }
}

void cs_explain_hook_cb_comp(g_s *g, obj_s *o, void *ctx)
{
    cs_s              *cs = (cs_s *)ctx;
    cs_explain_hook_s *dm = (cs_explain_hook_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    case 1: {
        cs->phase++;
        cs->tick = 0;
        puppet_set_anim(dm->puppet_comp, PUPPET_COMPANION_ANIMID_NOD_ONCE,
                        -dm->puppet_hero->facing);
        dia_load_from_wad(g, "D_EXPLAIN_HOOK");
        break;
    }
    }
}

void cs_explain_hook_on_trigger(g_s *g, cs_s *cs, i32 trigger)
{
    cs_explain_hook_s *dm = (cs_explain_hook_s *)cs->mem;

    switch (trigger) {
    case TRIGGER_DIA_END: {
        cs->phase++;
        cs->tick = 0;
        break;
    }
    }
}