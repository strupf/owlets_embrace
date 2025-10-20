// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_aquire_heartpiece_update(g_s *g, cs_s *cs, inp_s inp);
void cs_aquire_heartpiece_draw(g_s *g, cs_s *cs, v2_i32 cam);

void cs_aquire_heartpiece_enter(g_s *g, bool32 is_stamina)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->counter0  = is_stamina;
    cs->on_update = cs_aquire_heartpiece_update;
    cs->on_draw   = cs_aquire_heartpiece_draw;
    g->flags |= GAME_FLAG_BLOCK_PLAYER_INPUT;
}

void cs_aquire_heartpiece_update(g_s *g, cs_s *cs, inp_s inp)
{
    cs->tick++;
    switch (cs->phase) {
    case 0:
        if (owl_wait_for_idle(g)) {
            cs->phase++;
            cs->tick  = 0;
            cs->p_owl = puppet_owl_put(g, obj_get_owl(g));
            puppet_set_anim(cs->p_owl, PUPPET_OWL_ANIMID_PRESENT_ABOVE, 0);
        }
        break;
    case 1:
        if (100 <= cs->tick) {
            cs->phase++;
            cs->tick = 0;
            puppet_set_anim(cs->p_owl, PUPPET_OWL_ANIMID_PRESENT_ABOVE_TO_IDLE, 0);
        }
        break;
    case 2:
        if (20 <= cs->tick) {
            puppet_owl_replace_and_del(g, obj_get_owl(g), cs->p_owl);
            cs_reset(g);
            g->flags &= ~GAME_FLAG_BLOCK_PLAYER_INPUT;
        }
        break;
    }
}

void cs_aquire_heartpiece_draw(g_s *g, cs_s *cs, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();

    switch (cs->phase) {
    case 1: {
        v2_i32 ph = obj_pos_center(cs->p_owl);
        ph.x -= 16;
        ph.y -= 64;
        texrec_s trfeather = asset_texrec(TEXID_FEATHERUPGR, cs->counter0 * 32, 0, 32, 32);
        ph                 = v2_i32_add(ph, cam);
        gfx_spr(ctx, trfeather, ph, 0, 0);
        break;
    }
    }
}