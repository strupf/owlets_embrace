// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void cs_aquire_heartpiece_update(g_s *g, cs_s *cs);
void cs_aquire_heartpiece_draw(g_s *g, cs_s *cs, v2_i32 cam);

void cs_aquire_heartpiece_enter(g_s *g, bool32 is_stamina)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->counter0         = is_stamina;
    cs->on_update        = cs_aquire_heartpiece_update;
    cs->on_draw          = cs_aquire_heartpiece_draw;
    g->block_owl_control = 1;
}

void cs_aquire_heartpiece_update(g_s *g, cs_s *cs)
{
    cs->tick++;
    switch (cs->phase) {
    case 0:
        if (cs_wait_and_pause_for_owl_idle(g)) {
            cs->phase++;
            cs->tick  = 0;
            cs->p_owl = puppet_owl_put(g, obj_get_owl(g));
            puppet_set_anim(cs->p_owl, PUPPET_OWL_ANIMID_PRESENT_ABOVE, 0);
        }
        break;
    case 1:
        if (100 <= cs->tick) {
            puppet_owl_replace_and_del(g, obj_get_owl(g), cs->p_owl);
            cs_reset(g);
            g->block_owl_control = 0;
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