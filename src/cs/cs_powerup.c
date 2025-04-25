// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    obj_s *o;
} cs_powerup_s;

void cs_powerup_update(g_s *g, cs_s *cs);
void cs_powerup_draw(g_s *g, cs_s *cs, v2_i32 cam);
void cs_powerup_draw_bg(g_s *g, cs_s *cs, v2_i32 cam);

void cs_powerup_enter(g_s *g)
{
    g->block_hero_control = 1;
    cs_s *cs              = &g->cuts;
    cs_reset(g);
    cs_powerup_s *pu       = (cs_powerup_s *)cs->mem;
    cs->on_update          = cs_powerup_update;
    cs->on_draw            = cs_powerup_draw;
    cs->on_draw_background = cs_powerup_draw_bg;
    pu->o                  = obj_find_ID(g, OBJID_HERO_UPGRADE, 0);
}

void cs_powerup_update(g_s *g, cs_s *cs)
{
    obj_s        *ocomp  = obj_get_tagged(g, OBJ_TAG_COMPANION);
    cs_powerup_s *pu     = (cs_powerup_s *)cs->mem;
    v2_i32        orbpos = hero_upgrade_orb_pos(pu->o);

    switch (cs->phase) {
    case 0: {
        if (cs_wait_and_pause_for_hero_idle(g)) { // wait until player movement stopped
            cs->phase++;
            cs->tick = 0;

            // move cam to a fixed position over time
            v2_i32 camc = obj_pos_bottom_center(pu->o);
            camc.y -= 106;
            g->cam.has_trg      = 1;
            g->cam.trg          = camc;
            g->cam.trg_fade_spd = 50;

            // move companion to position
            v2_i32 comppos = {orbpos.x - 40, orbpos.y};
            companion_cs_start(ocomp);
            companion_cs_set_anim(ocomp, COMPANION_CS_ANIM_FLY, +1);
            companion_cs_move_to(ocomp, comppos, 60, 0);
        }
        break;
    }
    case 1: {
        if (50 <= cs->tick) {
            cs->phase++;
            cs->tick = 0;

            // slowly move orb and companion upwards
            v2_i32 orbtarget = {orbpos.x, orbpos.y - 50};
            v2_i32 comppos   = {orbtarget.x - 40, orbtarget.y};
            companion_cs_start(ocomp);
            companion_cs_set_anim(ocomp, COMPANION_CS_ANIM_FLY, +1);
            companion_cs_move_to(ocomp, comppos, 50, 0);

            hero_upgrade_move_orb_to(pu->o, orbtarget, 50);
            hero_upgrade_put_orb_infront(pu->o);
        }
        break;
    }
    case 2: {
        if (100 <= cs->tick) {
            cs_reset(g);
            g->block_hero_control = 0;
            g->cam.has_trg        = 0;
        }
        break;
    }
    }
}

void cs_powerup_draw_bg(g_s *g, cs_s *cs, v2_i32 cam)
{
    cs_powerup_s *pu     = (cs_powerup_s *)cs->mem;
    v2_i32        orbpos = v2_i32_add(hero_upgrade_orb_pos(pu->o), cam);
    gfx_ctx_s     ctx    = gfx_ctx_display();
    rec_i32       r      = {0, 0, 400, 240};
    tex_s         t      = asset_tex(TEXID_DISPLAY_TMP_MASK);
    gfx_ctx_s     ctxt   = gfx_ctx_default(t);
    tex_clr(t, GFX_COL_CLEAR);
    gfx_rec_fill(ctxt, r, PRIM_MODE_BLACK);

#define CS_POWERUP_N_CIR 8
    for (i32 n = 0; n < CS_POWERUP_N_CIR; n++) {
        gfx_ctx_s cc = ctxt;
        cc.pat       = gfx_pattern_interpolate(CS_POWERUP_N_CIR - n, CS_POWERUP_N_CIR);
        i32 d        = ease_out_quad(50, 300, n, CS_POWERUP_N_CIR);
        gfx_cir_fill(cc, orbpos, d, PRIM_MODE_WHITE);
    }
    gfx_cir_fill(ctxt, orbpos, 50, PRIM_MODE_BLACK);

    switch (cs->phase) {
    case 1: {

        break;
    }
    }

    gfx_spr(ctx, texrec_from_tex(t), (v2_i32){0}, 0, 0);
}

void cs_powerup_draw(g_s *g, cs_s *cs, v2_i32 cam)
{
    cs_powerup_s *pu     = (cs_powerup_s *)cs->mem;
    v2_i32        orbpos = v2_i32_add(hero_upgrade_orb_pos(pu->o), cam);
    gfx_ctx_s     ctx    = gfx_ctx_display();

    switch (cs->phase) {
    case 1: {

        break;
    }
    }
}
