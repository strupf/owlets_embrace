// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero_powerup.h"
#include "game.h"

enum {
    POWERUP_PHASE_NONE,
    POWERUP_PHASE_GAME_TO_WHITE,
    POWERUP_PHASE_WHITE_TO_BLACK,
    POWERUP_PHASE_TEXT,
    POWERUP_PHASE_TEXT_INPUT,
    POWERUP_PHASE_BACK_TO_GAME,
    //
    NUM_POWERUP_PHASES
};

const u8 powerup_phase[NUM_POWERUP_PHASES] = {
    0,
    35,
    30,
    30,
    30,
    40};

#define NUM_POWERUP_TEXTS 16
#define NUM_POWERUP_LINES 8

typedef struct {
    ALIGNAS(4)
    char title[32];
    char txt[NUM_POWERUP_LINES][64];
} powerup_text_s;

extern const powerup_text_s powerup_texts[NUM_POWERUP_TEXTS];

void hero_powerup_draw_text(gfx_ctx_s ctx, i32 ID);

void hero_powerup_collected(g_s *g, i32 ID)
{
    obj_s          *o  = obj_get_tagged(g, OBJ_TAG_HERO);
    hero_s         *h  = (hero_s *)o->heap;
    hero_powerup_s *pu = &g->powerup;
    pu->ID             = ID;
    pu->phase          = 1;
    pu->tick           = 0;
    pu->tick_total     = 0;
    g->substate        = SUBSTATE_POWERUP;

    switch (ID) {
    case HERO_UPGRADE_FLY:
    case HERO_UPGRADE_CLIMB:
        if (!g->hero.stamina_upgrades) {
            g->hero.stamina_upgrades = 1;
        }
        break;
    }
    pltf_log("%i\n", g->hero.stamina_upgrades);
    hero_add_upgrade(g, ID);
    snd_play(SNDID_UPGRADE, 1.f, 1.f);
}

void hero_powerup_update(g_s *g)
{
    hero_powerup_s *pu = &g->powerup;
    pu->tick_total++;

    if (pu->phase == POWERUP_PHASE_TEXT_INPUT && pu->tick == 0) {
        if (inp_btn_jp(INP_A)) {
            pu->tick++;
        }
        return;
    }

    pu->tick++;
    if (powerup_phase[pu->phase] <= pu->tick) {
        pu->tick = 0;
        pu->phase++;

        if (pu->phase == NUM_POWERUP_PHASES) {
            pu->phase   = 0;
            g->substate = 0;
        }
    }
}

void hero_powerup_draw(g_s *g, v2_i32 cam)
{
    hero_powerup_s *pu       = &g->powerup;
    tex_s           tdisplay = asset_tex(0);
    gfx_ctx_s       ctx      = gfx_ctx_default(tdisplay);

    rec_i32   rdisplay = {0, 0, PLTF_DISPLAY_W, PLTF_DISPLAY_H};
    const i32 T        = powerup_phase[pu->phase];

    spm_push();
    tex_s ttmp = tex_create(PLTF_DISPLAY_W, PLTF_DISPLAY_H, 1, spm_allocator(), 0);
    tex_clr(ttmp, GFX_COL_CLEAR);
    gfx_ctx_s ctxtmp = gfx_ctx_default(ttmp);

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    assert(ohero);
    v2_i32 pcirc = v2_i32_add(cam, obj_pos_center(ohero));

    switch (pu->phase) {
    default: break;
    case POWERUP_PHASE_GAME_TO_WHITE: {
        gfx_ctx_s ctxtmp_rec = ctxtmp;
        ctxtmp_rec.pat       = gfx_pattern_interpolate(pu->tick, T);
        gfx_rec_fill(ctxtmp_rec, rdisplay, GFX_COL_WHITE);
        i32 dcirc = ease_lin(0, 800, pu->tick, T);
        gfx_cir_fill(ctxtmp, pcirc, dcirc, GFX_COL_CLEAR);
        break;
    }
    case POWERUP_PHASE_WHITE_TO_BLACK: {
        gfx_rec_fill(ctxtmp, rdisplay, GFX_COL_WHITE);
        break;
    }
    }

    // draw hero animation
    if (pu->phase <= POWERUP_PHASE_WHITE_TO_BLACK) {
        v2_i32   phero   = {pcirc.x - 32, pcirc.y - 64};
        i32      frameID = min_i32((pu->tick_total * 2) / 5, 23);
        i32      xf      = frameID % 12;
        i32      yf      = 18 + (frameID / 12);
        texrec_s th      = asset_texrec(TEXID_HERO, xf * 64, yf * 64, 64, 64);
        gfx_spr(ctxtmp, th, phero, 0, 0);
    }

    switch (pu->phase) {
    default: break;
    case POWERUP_PHASE_WHITE_TO_BLACK: {
        i32 hrec = ease_lin(0, PLTF_DISPLAY_H / 2, pu->tick, T);

        rec_i32 r1 = {0, 0, PLTF_DISPLAY_W, hrec};
        rec_i32 r2 = {0, PLTF_DISPLAY_H - hrec, PLTF_DISPLAY_W, hrec};
        gfx_rec_fill(ctxtmp, r1, GFX_COL_BLACK);
        gfx_rec_fill(ctxtmp, r2, GFX_COL_BLACK);
        break;
    }
    case POWERUP_PHASE_TEXT: {
        gfx_rec_fill(ctxtmp, rdisplay, GFX_COL_BLACK);
        gfx_ctx_s ctxtmp_txt = ctxtmp;
        ctxtmp_txt.pat       = gfx_pattern_interpolate(pu->tick, T);
        hero_powerup_draw_text(ctxtmp_txt, pu->ID);
        break;
    }
    case POWERUP_PHASE_TEXT_INPUT: {
        gfx_rec_fill(ctxtmp, rdisplay, GFX_COL_BLACK);

        gfx_ctx_s ctxtmp_txt = ctxtmp;
        ctxtmp_txt.pat       = gfx_pattern_interpolate(pu->tick, T);
        ctxtmp_txt.pat       = gfx_pattern_inv(ctxtmp_txt.pat);
        hero_powerup_draw_text(ctxtmp_txt, pu->ID);

        i32 frb = 0;
        if (pu->tick == 0) { // waiting for button press -> animate
            i32 ff = (pu->tick_total / 3) & 15;
            switch (ff) {
            case 13:
            case 14: frb = 2; break;
            case 12:
            case 15: frb = 1; break;
            }
        }

        texrec_s trbut = asset_texrec(TEXID_UI, 0, frb * 32, 32, 32);
        v2_i32   pbut  = {350, 200};
        gfx_spr(ctxtmp_txt, trbut, pbut, 0, 0);

        break;
    }
    case POWERUP_PHASE_BACK_TO_GAME: {
        ctxtmp.pat = gfx_pattern_interpolate(pu->tick, T);
        ctxtmp.pat = gfx_pattern_inv(ctxtmp.pat);
        gfx_rec_fill(ctxtmp, rdisplay, GFX_COL_BLACK);
        break;
    }
    }

    texrec_s trtmp = {ttmp, rdisplay.x, rdisplay.y, rdisplay.w, rdisplay.h};
    gfx_spr(ctx, trtmp, CINIT(v2_i32){0}, 0, 0);
    spm_pop();
}

void hero_powerup_draw_text(gfx_ctx_s ctx, i32 ID)
{
    fnt_s       fnt   = asset_fnt(FNTID_MEDIUM);
    const char *text1 = powerup_texts[ID].title;
    i32         len1  = fnt_length_px(fnt, text1);
    v2_i32      lpos1 = {(400 - len1) / 2, 30};
    fnt_draw_str(ctx, fnt, lpos1, text1, SPR_MODE_WHITE);

    for (i32 n = 0; n < NUM_POWERUP_LINES; n++) {
        const char *text = powerup_texts[ID].txt[n];
        i32         len  = fnt_length_px(fnt, text);
        v2_i32      lpos = {(400 - len) / 2, 80 + n * 30};
        fnt_draw_str(ctx, fnt, lpos, text, SPR_MODE_WHITE);
    }
}

const powerup_text_s powerup_texts[NUM_POWERUP_TEXTS];