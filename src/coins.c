// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "coins.h"
#include "game.h"

// returns number of digits
void coins_draw_digits(gfx_ctx_s ctx, u32 n, v2_i32 pos);

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
    coins_s *c     = &g->coins;
    c->bop_changed = 0;

    if (c->show_idle) {
        c->show_idle_tick = min_i32(c->show_idle_tick + 1, 255);
    } else if (c->show_idle_tick) { // was idle
        c->show_idle_tick = 0;
        c->fade_out_tick  = 0; // fade out immediately
    }

    // fade in
    if (c->n_change) {
        c->fade_q7 = min_i32(c->fade_q7 + 30, 128);
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
            c->bop_changed = (c->n_change & 3);
            if (c->bop_changed == 3) {
                c->bop_changed = 1;
            }

            if (c->n_change == 0) {
                c->fade_out_tick = 100;
            }
        }
    } else if (c->fade_q7 && !c->show_idle) {
        // fade out
        if (c->fade_out_tick) {
            c->fade_out_tick--;
        } else {
            c->fade_q7 = max_i32(c->fade_q7 - 20, 0);
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
    coins_s  *c     = &g->coins;
    gfx_ctx_s ctx   = gfx_ctx_display();
    ctx.pat         = gfx_pattern_interpolate(c->fade_q7, 128);
    texrec_s tricon = asset_texrec(TEXID_BUTTONS, 96, 224, 32, 32);
    v2_i32   pos    = {369, 7};               // positon sprite
    v2_i32   posn   = {pos.x - 6, pos.y + 5}; // position n coins
    pos.x += ease_out_quad(20, 0, c->fade_q7, 128);
    texrec_s trnum = asset_texrec(TEXID_BUTTONS, 128, 320, 9, 12);

    gfx_spr(ctx, tricon, pos, 0, 0);
    coins_draw_digits(ctx, (u32)c->n, posn);

    if (c->n_change) {
        v2_i32 posc = {posn.x, posn.y + 20}; // position change coins
        posc.y -= c->bop_changed << 1;
        v2_i32 poss = {posn.x - 32, posc.y}; // position change coins
        coins_draw_digits(ctx, (u32)abs_i32(c->n_change), posc);

        // sign
        texrec_s trsig = asset_texrec(TEXID_BUTTONS,
                                      224 + (c->n_change < 0) * 16,
                                      320, 16, 16);
        gfx_spr(ctx, trsig, poss, 0, 0);
    }
}

void coins_show_idle(g_s *g)
{
    g->coins.show_idle = 1;
}

void coins_draw_digits(gfx_ctx_s ctx, u32 n, v2_i32 pos)
{
    // draw mono digit font
    v2_i32   p = pos;
    texrec_s t = asset_texrec(TEXID_BUTTONS, 128, 320, 9, 12);

    for (i32 k = 0, d = 1; k < 3; k++, d *= 10) {
        i32 c = n / d;
        if (c == 0 && 0 < k) { // at least render "0"
            break;
        }

        t.x = 128 + (c % 10) * 9;
        gfx_spr(ctx, t, p, 0, 0);
        p.x -= 10;
    }
}