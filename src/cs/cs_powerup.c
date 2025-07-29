// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// cutscene where the player aquires an upgrade

#include "game.h"

typedef struct {
    obj_s *o;
    obj_s *puppet_hero;
    obj_s *puppet_comp;
    bool32 first_time_seen;
} cs_powerup_s;

#define CS_POWERUP_TICKS_P10  200
#define CS_POWERUP_TICKS_P11  100
#define CS_POWERUP_TICKS_P13  20
#define CS_POWERUP_TICKS_P14  50
#define CS_POWERUP_TICKS_P15  40
#define CS_POWERUP_TICKS_P16  50
#define CS_POWERUP_TICKS_P50  50
#define CS_POWERUP_TICKS_P54  25
#define CS_POWERUP_TICKS_P55  50
#define CS_POWERUP_TICKS_P200 20

void cs_powerup_leave(g_s *g);
void cs_powerup_update(g_s *g, cs_s *cs);
void cs_powerup_draw(g_s *g, cs_s *cs, v2_i32 cam);
void cs_powerup_draw_bg(g_s *g, cs_s *cs, v2_i32 cam);
void cs_powerup_on_trigger(g_s *g, cs_s *cs, i32 trigger);

void cs_powerup_on_trigger(g_s *g, cs_s *cs, i32 trigger)
{
    cs_powerup_s *pu = (cs_powerup_s *)cs->mem;
    switch (cs->phase) {
    case 1: {
        if (trigger == TRIGGER_DIALOG_END) {
            puppet_set_anim(pu->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, +1);
            cs->phase++;
            cs->tick = 0;
        }
        break;
    }
    }
}

void cs_arrival_hero_cb(g_s *g, obj_s *o, void *ctx)
{
    cs_s         *cs     = (cs_s *)ctx;
    cs_powerup_s *pu     = (cs_powerup_s *)cs->mem;
    v2_i32        orbpos = hero_upgrade_orb_pos(pu->o);

    switch (cs->counter1) {
    case 1:
        puppet_set_anim(pu->puppet_hero, PUPPET_OWL_ANIMID_IDLE, 0);
        break;
    }
}

void cs_powerup_enter(g_s *g)
{
    g->block_owl_control = 1;
    cs_s *cs             = &g->cs;
    cs_reset(g);
    cs_powerup_s *pu       = (cs_powerup_s *)cs->mem;
    cs->on_update          = cs_powerup_update;
    cs->on_draw            = cs_powerup_draw;
    cs->on_draw_background = cs_powerup_draw_bg;
    cs->on_trigger         = cs_powerup_on_trigger;
    obj_s *oupgr           = obj_find_ID(g, OBJID_HERO_UPGRADE, 0);
    hero_upgrade_collect(g, oupgr);
    pu->o               = oupgr;
    pu->first_time_seen = save_event_exists(g, SAVE_EV_CS_POWERUP_FIRST_TIME);

    obj_s *ocomp = obj_get_tagged(g, OBJ_TAG_COMPANION);
    if (ocomp) {
        save_event_register(g, SAVE_EV_CS_POWERUP_FIRST_TIME);
    }
}

void cs_powerup_update(g_s *g, cs_s *cs)
{
    obj_s        *ocomp = obj_get_tagged(g, OBJ_TAG_COMPANION);
    cs_powerup_s *pu    = (cs_powerup_s *)cs->mem;
    v2_i32        opos  = hero_upgrade_orb_pos(pu->o);
    v2_i32        hpos  = {0};
    if (pu->puppet_hero) {
        hpos = pu->puppet_hero->pos;
    }

    switch (cs->phase) {
    case 0: {
        if (cs_wait_and_pause_for_owl_idle(g)) { // wait until player movement stopped

            // move cam to a fixed position over time
            v2_i32 camc = obj_pos_bottom_center(pu->o);
            camc.y -= 106;
            g->cam.has_trg      = 1;
            g->cam.trg          = camc;
            g->cam.trg_fade_spd = 50;

            // create hero puppet, freeze and hide the actual object
            obj_s *ohero    = obj_get_tagged(g, OBJ_TAG_OWL);
            v2_i32 hctrp    = obj_pos_center(ohero);
            pu->puppet_hero = puppet_owl_put(g, ohero);
            i32 hfacing     = camc.x < hctrp.x ? -1 : +1;
            puppet_set_anim(pu->puppet_hero, PUPPET_OWL_ANIMID_IDLE, hfacing);

            // if present: create companion puppet
            if (ocomp) {
                pu->puppet_comp = puppet_companion_put(g, ocomp);

#if 0
                if (g->hero.mode == HERO_MODE_COMBAT) {
                    hctrp.y -= 8;
                    hctrp.x -= hfacing * 8;
                    pu->puppet_comp->pos = hctrp;
                }
#endif

                if (!pu->first_time_seen) {
                    // move companion puppet to position
                    v2_i32 comppos = {opos.x - 40, opos.y - 0};
                    puppet_move(pu->puppet_comp, comppos, 40);
                }
                puppet_set_anim(pu->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, hfacing);
            }

            if (pu->first_time_seen || !pu->puppet_comp) {
                cs->phase = 3;
            } else {
                cs->phase++;
            }
            cs->tick = 0;
        }
        break;
    }
    case 1: {
        if (40 == cs->tick) {
            // dialog_open_wad(g, "D_UPGR_1");
            g->dialog.pos = DIALOG_POS_TOP;
            puppet_set_anim(pu->puppet_comp, PUPPET_COMPANION_ANIMID_HUH, +1);
        }
        if (150 == cs->tick) {
            // puppet_set_anim(pu->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, +1);
            cs->phase++;
            cs->tick = 0;
        }
        break;
    }
    case 2: {
        if (10 == cs->tick) {
            cs->phase++;
            cs->tick = 0;
        }
        break;
    }
    case 3: {
        cs->phase = 10;
        cs->tick  = 0;

        puppet_set_anim(pu->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, +1);

        // slowly move orb and companion upwards
        v2_i32 orbtarget = {opos.x, opos.y - 50};
        v2_i32 comppos   = {orbtarget.x - 40, orbtarget.y - 10};
        hero_upgrade_move_orb_to(pu->o, orbtarget, 150);
        if (ocomp && !pu->first_time_seen) {
            puppet_move(pu->puppet_comp, comppos, 150);
            puppet_set_anim(pu->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, 0);
        }
        break;
    }
    case 10: {
        switch (cs->tick) {
        case 30:
            hero_upgrade_put_orb_infront(pu->o);
            break;
        case 130:
            mus_play_extv(0, 0, 0, 2000, 0, 0);
            pu->puppet_hero->render_priority = RENDER_PRIO_UI_LEVEL;
            g->objrender_dirty               = 1;
            break;
        case CS_POWERUP_TICKS_P10:
            cs->phase++;
            cs->tick = 0;
            break;
        }
        break;
    }
    case 11: {
        if (CS_POWERUP_TICKS_P11 <= cs->tick) {
            cs->phase++;
            cs->tick = 0;
        }
        break;
    }
    case 12: {
        if (100 <= cs->tick) {
            cs->phase++;
            cs->tick = 0;
            if (ocomp && !pu->first_time_seen) {
                v2_i32 comppos = {opos.x - 150, opos.y + 20};
                puppet_move_ext(pu->puppet_comp, comppos, 18, ease_in_out_quad, 0, 0, 0);
            }
        }
        break;
    }
    case 13: {
        if (CS_POWERUP_TICKS_P13 <= cs->tick) {
            cs->phase++;
            cs->tick                         = 0;
            pu->puppet_hero->render_priority = RENDER_PRIO_DEFAULT_OBJ;
            g->objrender_dirty               = 1;
        }
        break;
    }
    case 14: {
        switch (cs->tick) {
        case 50: {
            snd_play(SNDID_UPGRADE, 1.f, 1.f);
            v2_i32 comppos = {opos.x - 100, opos.y + 60};
            v2_i32 heropos = {0, -30};
            puppet_set_anim(pu->puppet_hero, PUPPET_OWL_ANIMID_UPGR_RISE, 0);
            puppet_move_ext(pu->puppet_hero, heropos, 100, 0, 1, 0, 0);
            if (ocomp) {
                puppet_move_ext(pu->puppet_comp, comppos, 30, ease_in_out_quad, 0, 0, 0);
            }
            break;
        }
        case 100: {
            puppet_set_anim(pu->puppet_hero, PUPPET_OWL_ANIMID_UPGR_INTENSE, 0);
            break;
        }
        case 220: {
            v2_i32 hpos_mid = {hpos.x, hpos.y - 20};
            hero_upgrade_move_orb_to(pu->o, hpos_mid, 60);
            break;
        }
        case 240: {
            cs->phase++;
            cs->tick = 0;
            break;
        }
        }
        break;
    }
    case 15: {
        if (CS_POWERUP_TICKS_P15 == cs->tick) {
            cs->phase++;
            cs->tick = 0;
            puppet_set_anim(pu->puppet_hero, PUPPET_OWL_ANIMID_UPGR_CALM, 0);
            hero_upgrade_disable_orb(pu->o);
        }
        break;
    }
    case 16: {
        if (CS_POWERUP_TICKS_P16 == cs->tick) {
            cs->phase = 50;
            cs->tick  = 0;
        }
        break;
    }
    case 50: {
        if (CS_POWERUP_TICKS_P50 == cs->tick) {
            cs->phase++;
            cs->tick = 0;
        }
        break;
    }
    case 51: {
        cs->phase = 53;
        cs->tick  = 0;
        break;
    }
    case 53: {
        if (inp_btn_jp(INP_A)) {
            cs->phase++;
            cs->tick = 0;
        }
        break;
    }
    case 54: {
        if (CS_POWERUP_TICKS_P54 == cs->tick) {
            game_cue_area_music(g);
            cs->phase++;
            cs->tick = 0;
        }
        break;
    }
    case 55: {
        // back to "normal"
        if (CS_POWERUP_TICKS_P55 == cs->tick) {
            cs->phase      = 200;
            cs->tick       = 0;
            v2_i32 heropos = {0, +30};
            cs->counter1   = 1;
            puppet_move_ext(pu->puppet_hero, heropos, 100, 0, 1, cs_arrival_hero_cb, cs);
        }
        break;
    }
    case 200: {
        if (CS_POWERUP_TICKS_P200 == cs->tick) {
            cs_powerup_leave(g);

            obj_s *op = obj_find_ID(g, OBJID_HERO_UPGRADE, 0);
            switch (op->substate) {
            case 0: { // grappling hook
                cs_explain_hook_enter(g);
                break;
            }
            case 4: { // climbing
                break;
            }
            }
        }
        break;
    }
    }
}

void cs_powerup_leave(g_s *g)
{
    cs_s         *cs    = &g->cs;
    cs_powerup_s *pu    = (cs_powerup_s *)cs->mem;
    obj_s        *ohero = obj_get_owl(g);
    obj_s        *ocomp = obj_get_tagged(g, OBJ_TAG_COMPANION);

    puppet_owl_replace_and_del(g, ohero, pu->puppet_hero);
    if (ocomp) {
        puppet_companion_replace_and_del(g, ocomp, pu->puppet_comp);
    }

#if 0
    if (hero_has_upgrade(g, HERO_UPGRADE_HOOK)) {
        g->hero.mode = HERO_MODE_NORMAL;
    }
#endif
    g->block_owl_control = 0;
    g->cam.has_trg       = 0;
    cs_reset(g);
}

void cs_powerup_draw_bg(g_s *g, cs_s *cs, v2_i32 cam)
{
    cs_powerup_s *pu         = (cs_powerup_s *)cs->mem;
    v2_i32        orbpos     = v2_i32_add(hero_upgrade_orb_pos(pu->o), cam);
    gfx_ctx_s     ctx        = gfx_ctx_display();
    rec_i32       r          = {0, 0, 400, 240};
    gfx_ctx_s     ctxt       = ctx;
    gfx_ctx_s     ctxrecfill = ctxt;

#define CS_POWERUP_N_CIR 8

    gfx_ctx_s ctxc     = ctx;
    i32       d_shine1 = 270 + ((10 * sin_q15(g->tick_animation << 9)) / 32769);
    i32       d_shine2 = (d_shine1 * 150) >> 8;
    i32       d_shine3 = (d_shine1 * 270) >> 8;
    i32       d_shine  = 0;

    switch (cs->phase) {
    default:
        ctxrecfill.pat = gfx_pattern_0();
        break;
    case 10:
        ctxrecfill.pat = gfx_pattern_interpolate(cs->tick, CS_POWERUP_TICKS_P10);
        break;
    case 11:
        d_shine = lerp_i32(0, d_shine2, cs->tick, CS_POWERUP_TICKS_P11);
        break;
    case 12:
        d_shine = d_shine2;
        break;
    case 13:
        if (cs->tick < CS_POWERUP_TICKS_P13 / 2) {
            d_shine = ease_in_out_quad(d_shine2, d_shine3,
                                       cs->tick, CS_POWERUP_TICKS_P13 / 2);
        } else {
            d_shine = lerp_i32(d_shine3, d_shine1,
                               cs->tick - CS_POWERUP_TICKS_P13 / 2, CS_POWERUP_TICKS_P13 / 2);
        }
        break;
    case 14:
    case 15:
    case 16:
        d_shine = d_shine1;
        break;
    }

    gfx_rec_fill(ctxrecfill, r, PRIM_MODE_BLACK);

    if (d_shine) {
        for (i32 n = 0; n < CS_POWERUP_N_CIR; n++) {
            gfx_ctx_s cc    = ctxt;
            i32       patID = ease_lin(0, GFX_PATTERN_MAX,
                                       CS_POWERUP_N_CIR - n - 1, CS_POWERUP_N_CIR);
            cc.pat          = gfx_pattern_bayer_4x4(patID);
            i32 d           = ease_out_quad(50, d_shine, n, CS_POWERUP_N_CIR);
            gfx_cir_fill(cc, orbpos, d, PRIM_MODE_WHITE);
        }
        gfx_cir_fill(ctxt, orbpos, 50, PRIM_MODE_BLACK);
    }
}

void cs_powerup_draw(g_s *g, cs_s *cs, v2_i32 cam)
{
    cs_powerup_s *pu   = (cs_powerup_s *)cs->mem;
    v2_i32        opos = v2_i32_add(hero_upgrade_orb_pos(pu->o), cam);
    gfx_ctx_s     ctx  = gfx_ctx_display();

    i32 d_shine          = 0;
    i32 d_shine_1        = 350 + ((14 * sin_q15(g->tick_animation << 9)) / 32769);
    i32 d_shine_2        = d_shine_1 / 2;
    i32 white_overlay_q6 = 0;
    i32 text_q6          = 0;

    switch (cs->phase) {
    case 10:
        d_shine = lerp_i32(700, d_shine_2, cs->tick, CS_POWERUP_TICKS_P10);
        break;
    case 11:
    case 12:
        d_shine = d_shine_2;
        break;
    case 13:
        d_shine = lerp_i32(d_shine_2, d_shine_1, cs->tick, CS_POWERUP_TICKS_P13);
        break;
    case 14:
        d_shine = d_shine_1;
        break;
    case 15:
        d_shine          = d_shine_1;
        white_overlay_q6 = lerp_i32(0, 64, cs->tick, CS_POWERUP_TICKS_P15);
        break;
    case 16:
        white_overlay_q6 = 64;
        break;
    case 50:
        text_q6          = lerp_i32(0, 64, cs->tick, CS_POWERUP_TICKS_P50);
        white_overlay_q6 = lerp_i32(64, 48, cs->tick, CS_POWERUP_TICKS_P50);
        break;
    case 51:
    case 52:
    case 53:
        text_q6          = 64;
        white_overlay_q6 = 48;
        break;
    case 54:
        text_q6          = lerp_i32(64, 0, cs->tick, CS_POWERUP_TICKS_P54);
        white_overlay_q6 = 48;
        break;
    case 55:
        white_overlay_q6 = lerp_i32(48, 0, cs->tick, CS_POWERUP_TICKS_P54);
        break;
    }

    // orb light shine
    if (d_shine) {
        tex_s     t    = asset_tex(TEXID_DISPLAY_TMP);
        gfx_ctx_s ctxt = gfx_ctx_default(t);
        tex_clr(t, GFX_COL_BLACK);
        ctxt.pat = gfx_pattern_50();
        gfx_cir_fill(ctxt, opos, (d_shine * 256) >> 8, PRIM_MODE_WHITE);
        ctxt.pat = gfx_pattern_bayer_4x4(11);
        gfx_cir_fill(ctxt, opos, (d_shine * 240) >> 8, PRIM_MODE_WHITE);
        ctxt.pat = gfx_pattern_100();
        gfx_cir_fill(ctxt, opos, (d_shine * 224) >> 8, PRIM_MODE_WHITE);
        gfx_spr(ctx, texrec_from_tex(t), (v2_i32){0}, 0, SPR_MODE_BLACK_ONLY);
    }

    // overlay the scene with white
    if (white_overlay_q6) {
        rec_i32   rfull    = {0, 0, 400, 240};
        gfx_ctx_s ctxwhite = ctx;
        ctxwhite.pat       = gfx_pattern_interpolate(white_overlay_q6, 64);
        gfx_rec_fill(ctxwhite, rfull, PRIM_MODE_WHITE);
    }

    // upgrade text
    if (text_q6) {
        gfx_ctx_s ctext1 = ctx;
        ctext1.pat       = gfx_pattern_interpolate(text_q6, 64);
        fnt_s fnt1       = asset_fnt(FNTID_LARGE);
        fnt_s fnt2       = asset_fnt(FNTID_LARGE);

        obj_s *op = obj_find_ID(g, OBJID_HERO_UPGRADE, 0);

        switch (op->substate) {
        case 0: { // grappling hook
            fnt_draw_outline_style(ctext1, fnt1, (v2_i32){200, 20}, "You received the", 1, 1);
            fnt_draw_outline_style(ctext1, fnt2, (v2_i32){200, 50}, "Grappling Hook!", 1, 1);
            break;
        }
        case 4: { // climbing
            fnt_draw_outline_style(ctext1, fnt1, (v2_i32){200, 20}, "You received the", 1, 1);
            fnt_draw_outline_style(ctext1, fnt2, (v2_i32){200, 50}, "Grappling Hook!", 1, 1);
            break;
        }
        }
    }
}
