// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define UPGRADE_DESC_LINES 4

typedef struct {
    char name[32];
    char line[UPGRADE_DESC_LINES][64];
} upgrade_text_s;

static const upgrade_text_s g_upgrade_text[NUM_HERO_UPGRADES] =
    {
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"Air Jump",
         {"Your confidence grew. Maybe you can try to",
          "put those wings to use."}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"Grappling Hook",
         {"Maybe this tool can be",
          "put to good use!"}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
        {"Air Jump",
         {"Your confidence grew. Maybe you can try to",
          "put those wings to use."}},
        //
        {"High Jump",
         {"You grew stronger and can jump higher!"}},
        //
};

enum {
    UPGRADEHANDLER_TO_WHITE,
    UPGRADEHANDLER_BARS_TO_BLACK,
    UPGRADEHANDLER_TEXT_FADE_IN,
    UPGRADEHANDLER_TEXT,
    UPGRADEHANDLER_TEXT_WAIT,
    UPGRADEHANDLER_FADE_OUT,
    //
    UPGRADEHANDLER_NUM_PHASES
};

static int upgradehandler_phase_ticks(upgradehandler_s *h)
{
    static const int phase_ticks[UPGRADEHANDLER_NUM_PHASES] = {
        50, // to white
        20, // to black
        30, // text fade in
        0,  // wait for input
        10, // delay fade out
        30  // fade out
    };
    return phase_ticks[h->phase];
}

void upgradehandler_start_animation(upgradehandler_s *h, int upgrade)
{
    h->active  = 1;
    h->t       = 0;
    h->phase   = 0;
    h->upgrade = upgrade;
    snd_play_ext(SNDID_UPGRADE, 1.f, 2.f);
}

bool32 upgradehandler_in_progress(upgradehandler_s *h)
{
    return h->active;
}

void upgradehandler_tick(upgradehandler_s *h)
{
    if (h->phase == UPGRADEHANDLER_TEXT) {
        if (inp_just_pressed(INP_A)) {
            snd_play_ext(SNDID_SELECT, 0.5, 1.f);
        } else {
            return;
        }
    }

    const int ticks = upgradehandler_phase_ticks(h);
    h->t++;
    if (h->t < ticks) return;
    h->t = 0;
    h->phase++;
    if (h->phase < UPGRADEHANDLER_NUM_PHASES) return;
    h->phase  = 0;
    h->active = 0;
}

void upgradehandler_draw(game_s *g, upgradehandler_s *h, v2_i32 camoffset)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!ohero) return;

    const int       ticks = upgradehandler_phase_ticks(h);
    const gfx_ctx_s ctx   = gfx_ctx_display();
    gfx_ctx_s       ctx_1 = ctx;
    gfx_ctx_s       ctx_2 = ctx;

    const upgrade_text_s *ut       = &g_upgrade_text[h->upgrade];
    rec_i32               rdisplay = {0, 0, SYS_DISPLAY_W, SYS_DISPLAY_H};

    switch (h->phase) {
    case UPGRADEHANDLER_TO_WHITE: { // growing white circle
        ctx_1.pat      = gfx_pattern_interpolate(h->t, ticks);
        v2_i32 drawpos = obj_pos_center(ohero);
        drawpos        = v2_add(drawpos, camoffset);
        int dtt        = 3 * max_i(abs_i(drawpos.x),
                                   abs_i(SYS_DISPLAY_W - drawpos.x));
        int cd         = (dtt * h->t) / ticks;

        gfx_rec_fill(ctx_1, rdisplay, PRIM_MODE_WHITE);
        gfx_cir_fill(ctx, drawpos, cd, PRIM_MODE_WHITE);
        break;
    }
    case UPGRADEHANDLER_BARS_TO_BLACK: { // black bars closing to black screen
        gfx_rec_fill(ctx_1, rdisplay, PRIM_MODE_WHITE);
        int rh = ease_out_quad(0, 120, h->t, ticks);
        gfx_rec_fill(ctx_1,
                     (rec_i32){0, 0, SYS_DISPLAY_W, rh},
                     PRIM_MODE_BLACK);
        gfx_rec_fill(ctx_1,
                     (rec_i32){0, SYS_DISPLAY_H - rh, SYS_DISPLAY_W, rh},
                     PRIM_MODE_BLACK);
        break;
    }
    case UPGRADEHANDLER_FADE_OUT:       // fade back to gameplay; fallthrough
    case UPGRADEHANDLER_TEXT_FADE_IN: { // fade in upgrade text
        switch (h->phase) {
        case UPGRADEHANDLER_FADE_OUT:
            ctx_1.pat = gfx_pattern_interpolate(ticks - h->t, ticks);
            ctx_2     = ctx_1;
            break;
        case UPGRADEHANDLER_TEXT_FADE_IN:
            ctx_1.pat = gfx_pattern_interpolate(h->t, ticks);
            break;
        }
    } // fallthrough
    case UPGRADEHANDLER_TEXT_WAIT:
    case UPGRADEHANDLER_TEXT: { // show text
        gfx_rec_fill(ctx_2, rdisplay, PRIM_MODE_BLACK);

        fnt_s font1 = asset_fnt(FNTID_LARGE);
        fnt_s font2 = asset_fnt(FNTID_MEDIUM);
        int   l1    = fnt_length_px(font1, ut->name);

        fnt_draw_ascii(ctx_1, font1,
                       (v2_i32){(SYS_DISPLAY_W - l1) / 2, 60},
                       ut->name, SPR_MODE_WHITE);

        for (int i = 0; i < UPGRADE_DESC_LINES; i++) {
            int li = fnt_length_px(font2, ut->line[i]);
            fnt_draw_ascii(ctx_1, font2,
                           (v2_i32){(SYS_DISPLAY_W - li) / 2, 100 + i * 30},
                           ut->line[i], SPR_MODE_WHITE);
        }

        texrec_s tr_a = asset_texrec(TEXID_UI, 0, 0, 32, 32);
        tr_a.r.y      = 32 * ((sys_tick() >> 5) & 1);
        gfx_spr(ctx_1, tr_a, (v2_i32){350, 200}, 0, 0);
        break;
    }
    }
}
