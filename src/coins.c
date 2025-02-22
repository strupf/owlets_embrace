// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "coins.h"
#include "game.h"

void coins_change(g_s *g, i32 n)
{
    coins_s *c     = &g->coins;
    i32      n_new = clamp_i32(c->n + c->n_change + n, 0, COINS_MAX);
    c->n_change    = n_new - c->n;

    if (c->n_change) {
        c->ticks_change = -100; // ticks until change is added to total
    }
}

void coins_update(g_s *g)
{
    coins_s *c = &g->coins;

    if (c->show_idle) {
        c->show_idle_tick = min_i32(c->show_idle_tick + 1, 255);
    } else if (c->show_idle_tick) { // was idle
        c->show_idle_tick = 0;
        c->fade_out_tick  = 0; // fade out immediately
    }

    // fade in
    if (c->n_change) {
        c->fade_q7 = min_i32(c->fade_q7 + 50, 128);
    } else if (100 <= c->show_idle_tick) {
        c->fade_q7 = min_i32(c->fade_q7 + 20, 128);
    }

    if (c->ticks_change < 0) {
        c->ticks_change++;
    } else if (c->n_change) {
        c->ticks_change++;

        if (c->ticks_change & 1) {
            c->n++;
            c->n_change--;

            if (c->n_change == 0) {
                c->fade_out_tick = 100;
            }
        }
    } else if (c->fade_q7 && !c->show_idle) {
        // fade out
        if (c->fade_out_tick) {
            c->fade_out_tick--;
        } else {
            c->fade_q7 = max_i32(c->fade_q7 - 25, 0);
        }
    }

    c->show_idle = 0;
}

i32 coins_total(g_s *g)
{
    coins_s *c = &g->coins;
    return (c->n + c->n_change);
}

void coins_draw(g_s *g)
{
#define COIN_MONO_SPACING 8
#define COIN_POS_END_X    92

    gfx_ctx_s ctx = gfx_ctx_display();
    coins_s  *c   = &g->coins;
    ctx.pat       = gfx_pattern_interpolate(c->fade_q7, 128);

    spm_push();
    tex_s cointex = tex_create(100, 60, 1, spm_allocator2(), 0);
    tex_clr(cointex, GFX_COL_CLEAR);
    gfx_ctx_s ctxcoin   = gfx_ctx_default(cointex);
    fnt_s     f         = asset_fnt(FNTID_SMALL);
    u8        str_1[16] = {0};
    strs_from_u32(c->n, str_1);

    i32    len = fnt_length_px_mono(f, str_1, COIN_MONO_SPACING);
    v2_i32 pos = {COIN_POS_END_X - len, 8};
    fnt_draw_ascii_mono(ctxcoin, f, pos, str_1,
                        SPR_MODE_BLACK, COIN_MONO_SPACING);

    if (c->n_change) {
        u8 str_2[8] = {0};
        strs_from_u32(abs_i32(c->n_change), str_2);

        i32    len_change = fnt_length_px_mono(f, str_2, COIN_MONO_SPACING);
        v2_i32 pos_change = {COIN_POS_END_X - len_change, pos.y + 20};
        v2_i32 pos_sign   = {COIN_POS_END_X - 40, pos_change.y};
        fnt_draw_ascii_mono(ctxcoin, f, pos_change, str_2,
                            SPR_MODE_BLACK, COIN_MONO_SPACING);
        fnt_draw_ascii_mono(ctxcoin, f, pos_sign, 0 < c->n_change ? "+" : "-",
                            SPR_MODE_BLACK, COIN_MONO_SPACING);
    }
    tex_outline_white(cointex);
    tex_outline_white(cointex);
    texrec_s trcoin   = {cointex, 0, 0, 100, 60};
    v2_i32   labelpos = {300, 0};
    if (0 < c->ticks_change && c->n_change) {
        labelpos.y += c->ticks_change & 1;
    }
    labelpos.y += (6 * (128 - c->fade_q7)) / 128;
    gfx_spr(ctx, trcoin, labelpos, 0, 0);
    spm_pop();
}

void coins_show_idle(g_s *g)
{
    g->coins.show_idle = 1;
}