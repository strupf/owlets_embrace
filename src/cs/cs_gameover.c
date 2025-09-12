// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    CS_GAMEOVER_DYING,
    CS_GAMEOVER_GAMEOVER_FADE_IN,
    CS_GAMEOVER_GAMEOVER,
    CS_GAMEOVER_GAMEOVER_INPUT,
    CS_GAMEOVER_GAMEOVER_FADE_OUT,
    CS_GAMEOVER_BLACK
};

#define CS_GAMEOVER_TICKS_DYING             120
#define CS_GAMEOVER_TICKS_GAMEOVER_FADE_IN  40
#define CS_GAMEOVER_TICKS_GAMEOVER          70
#define CS_GAMEOVER_TICKS_GAMEOVER_INPUT    10
#define CS_GAMEOVER_TICKS_GAMEOVER_FADE_OUT 10
#define CS_GAMEOVER_TICKS_BLACK             30
#define CS_GAMEOVER_TICKS_FADE              25

void cs_gameover_update(g_s *g, cs_s *cs, inp_s inp);
void cs_gameover_draw(g_s *g, cs_s *cs, v2_i32 cam);

void cs_gameover_enter(g_s *g)
{
    cs_s *cs = &g->cs;
    cs_reset(g);
    cs->on_update = cs_gameover_update;
    cs->on_draw   = cs_gameover_draw;
}

void cs_gameover_update(g_s *g, cs_s *cs, inp_s inp)
{
    switch (cs->phase) {
    case CS_GAMEOVER_DYING: {
        if (CS_GAMEOVER_TICKS_DYING <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
        }
        break;
    }
    case CS_GAMEOVER_GAMEOVER_FADE_IN: {
        if (CS_GAMEOVER_TICKS_GAMEOVER_FADE_IN <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
        }
        break;
    }
    case CS_GAMEOVER_GAMEOVER: {
        if (CS_GAMEOVER_TICKS_GAMEOVER <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
        }
        break;
    }
    case CS_GAMEOVER_GAMEOVER_INPUT: {
        if (CS_GAMEOVER_TICKS_GAMEOVER_INPUT <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
        }
        break;
    }
    case CS_GAMEOVER_GAMEOVER_FADE_OUT: {
        if (CS_GAMEOVER_TICKS_GAMEOVER_FADE_OUT <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
        }
        break;
    }
    case CS_GAMEOVER_BLACK: {
        if (CS_GAMEOVER_TICKS_BLACK <= cs->tick) {
            cs->tick = 0;
            cs->phase++;
            mus_play_ext(0, 0, 0, 0, 100, 0, 0);
            g->music_ID = 0;
            aud_stop_all_snd_instances();
            game_load_savefile(g);
            cs_on_load_title_wakeup(g);
            cs_reset(g);
        }
        break;
    }
    }
}

void cs_gameover_draw(g_s *g, cs_s *cs, v2_i32 cam)
{
    gfx_ctx_s ctx        = gfx_ctx_display();
    rec_i32   rdisplay   = {0, 0, PLTF_DISPLAY_W, PLTF_DISPLAY_H};
    gfx_ctx_s ctx_r      = ctx;
    i32       p_gameover = 12;

    switch (cs->phase) {
    case CS_GAMEOVER_DYING: {
        i32 tt = (CS_GAMEOVER_TICKS_DYING * 3) / 4;
        if (tt <= cs->tick) {
            i32 p     = lerp_i32(0, p_gameover, cs->tick - tt, CS_GAMEOVER_TICKS_DYING - tt);
            ctx_r.pat = gfx_pattern_bayer_4x4(p);
            gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);
        }
        break;
    }
    case CS_GAMEOVER_GAMEOVER_FADE_IN:
    case CS_GAMEOVER_GAMEOVER:
    case CS_GAMEOVER_GAMEOVER_INPUT: {
        ctx_r.pat = gfx_pattern_bayer_4x4(p_gameover);
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);

        fnt_s font = asset_fnt(FNTID_MEDIUM);

        v2_i32 pos = {150, 100};
        for (i32 y = -2; y <= +2; y++) {
            for (i32 x = -2; x <= +2; x++) {
                v2_i32 p = pos;
                p.x += x;
                p.y += y;
                fnt_draw_str(ctx, font, p, "Game Over", SPR_MODE_WHITE);
            }
        }

        fnt_draw_str(ctx, font, pos, "Game Over", SPR_MODE_BLACK);
        break;
    }
    case CS_GAMEOVER_GAMEOVER_FADE_OUT: {
        i32 p     = lerp_i32(p_gameover, GFX_PATTERN_MAX,
                             cs->tick, CS_GAMEOVER_TICKS_GAMEOVER_FADE_OUT);
        ctx_r.pat = gfx_pattern_bayer_4x4(p);
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);
        break;
    }
    case CS_GAMEOVER_BLACK: {
        gfx_rec_fill(ctx_r, rdisplay, PRIM_MODE_BLACK);
        break;
    }
    }
}
