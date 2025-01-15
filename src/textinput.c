// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "textinput.h"
#include "core/assets.h"
#include "core/gfx.h"
#include "core/inp.h"
#include "util/easing.h"
#include "util/mathfunc.h"

typedef struct textinput_handler_s textinput_handler_s;

#ifdef PLTF_PD
#define TI_PD_COL_SPECIAL    0
#define TI_PD_COL_UPPER      1
#define TI_PD_COL_LOWER      2
#define TI_PD_COL_UTIL       3
#define TI_PD_NUM_UTIL       4
#define TI_PD_SPACE_Y        38
#define TI_PD_SCROLL_DELAY   15
#define TI_PD_A_SCROLL_MOD   5
#define TI_PD_Y_SCROLL_MOD   3
#define TI_PD_Y_SCROLL       6
#define TI_PD_OPEN_TICK      8
#define TI_PD_CLOSE_TICK     8
#define TI_PD_Y_SCROLL_TIMER 7

static const char g_kbrd[3][48] = {
    "!'\"#$%&()*+-/|\\[]^_`{}~@1234567890.,:;<=>?",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    "abcdefghijklmnopqrstuvwxyz"};

void textinput_pd_activate(textinput_handler_s *hdl);
void textinput_pd_deactivate(textinput_handler_s *hdl);
void textinput_pd_update(textinput_handler_s *hdl);
void textinput_pd_draw(textinput_handler_s *hdl);
#endif
#ifdef PLTF_SDL
void textinput_sdl_close_input(void *ctx);
void textinput_sdl_char_add(char c, void *ctx);
void textinput_sdl_char_del(void *ctx);
#endif

struct textinput_handler_s {
    textinput_s *tinp;
    bool16       active;
    bool16       only_letters;
#ifdef PLTF_PD
    inp_crank_click_s crank_click;

    u32  bp;
    u32  b;
    u32  a_hold_timer;
    u32  y_hold_timer;
    i8   pos_y[4];
    i8   pos_x;
    i8   open_tick;
    i8   close_tick;
    i8   x_scroll_timer;
    i8   y_scroll_timer;
    i8   x_bump_timer;
    i8   y_bump_timer;
    u8   saved_n;
    char saved_c[NUM_TEXTINPUT_CHARS];
#endif
};

static textinput_handler_s g_textinput_handler;

void textinput_add_char(textinput_s *txt, char c);
void textinput_del_char(textinput_s *txt);

void textinput_add_char(textinput_s *txt, char c)
{
    char ch = txt->on_char_add ? txt->on_char_add(c) : c;
    if (ch == '\0') return;

    i32 cap = txt->cap ? MIN(txt->cap, NUM_TEXTINPUT_CHARS - 1)
                       : NUM_TEXTINPUT_CHARS - 1;
    if (txt->n < cap) {
        txt->c[txt->n++] = c;
    }
}

void textinput_del_char(textinput_s *txt)
{
    if (txt->n) {
        txt->c[--txt->n] = '\0';
    }
}

bool32 textinput_active()
{
    textinput_handler_s *hdl = &g_textinput_handler;
    return hdl->active;
}

void textinput_activate(textinput_s *txt, bool32 only_letters)
{
    textinput_handler_s *hdl = &g_textinput_handler;
    hdl->active              = 1;
    hdl->tinp                = txt;
    hdl->only_letters        = only_letters;
#if defined(PLTF_PD)
    textinput_pd_activate(hdl);
#elif defined(PLTF_SDL)
    pltf_sdl_txt_inp_set_cb(textinput_sdl_char_add,
                            textinput_sdl_char_del,
                            textinput_sdl_close_input,
                            NULL);
#endif
}

void textinput_deactivate()
{
    textinput_handler_s *hdl = &g_textinput_handler;

#if defined(PLTF_PD)
    textinput_pd_deactivate(hdl);
#elif defined(PLTF_SDL)
    hdl->active = 0;
    pltf_sdl_txt_inp_clr_cb();
#endif
}

void textinput_update()
{
    if (!textinput_active()) return;
    textinput_handler_s *hdl = &g_textinput_handler;
#if defined(PLTF_PD)
    textinput_pd_update(hdl);
#elif defined(PLTF_SDL)

#endif
}

void textinput_draw()
{
#if defined(PLTF_PD)
    if (!textinput_active()) return;
    textinput_handler_s *hdl = &g_textinput_handler;
    textinput_pd_draw(hdl);
#endif
}

i32 textinput_draw_offs()
{
#if defined(PLTF_PD)
    textinput_handler_s *hdl = &g_textinput_handler;
    if (!textinput_active()) return 0;
    i32 of = hdl->only_letters ? 178 - 36 : 178;
    if (hdl->open_tick) {
        return ease_out_back(0, of, hdl->open_tick, TI_PD_OPEN_TICK);
    }
    if (hdl->close_tick) {
        return ease_out_back(0, of, hdl->close_tick, TI_PD_CLOSE_TICK);
    }
    return of;
#else
    return 0;
#endif
}

#if defined(PLTF_PD)
void textinput_pd_activate(textinput_handler_s *hdl)
{
    hdl->open_tick  = 1;
    hdl->close_tick = 0;
    hdl->saved_n    = hdl->tinp->n;
    mcpy(hdl->saved_c, hdl->tinp->c, NUM_TEXTINPUT_CHARS);

    hdl->pos_x                    = TI_PD_COL_UPPER;
    hdl->pos_y[TI_PD_COL_SPECIAL] = 24; // "1"
    hdl->pos_y[TI_PD_COL_UPPER]   = 0;  // "A"
    hdl->pos_y[TI_PD_COL_LOWER]   = 0;  // "a"
    hdl->pos_y[TI_PD_COL_UTIL]    = 1;  // "ok"
    inp_crank_click_init(&hdl->crank_click, 12, 1);
}

void textinput_pd_deactivate(textinput_handler_s *hdl)
{
    hdl->close_tick = TI_PD_CLOSE_TICK;
}

void textinput_pd_update(textinput_handler_s *hdl)
{
    hdl->bp     = hdl->b;
    hdl->b      = pltf_pd_btn();
    i32 crankdt = inp_crank_dt_q16();

    if (hdl->open_tick) {
        hdl->open_tick++;
        if (TI_PD_OPEN_TICK <= hdl->open_tick) {
            hdl->open_tick   = 0;
            i32 crankdt_init = inp_crank_q16(); // set first crank segment
            inp_crank_click_turn_by(&hdl->crank_click, crankdt_init);
        }
        return;
    }
    if (hdl->close_tick) {
        hdl->close_tick--;
        if (hdl->close_tick <= 0) {
            hdl->close_tick = 0;
            hdl->active     = 0;
        }
        return;
    }

    if (hdl->x_scroll_timer) {
        hdl->x_scroll_timer -= SGN(hdl->x_scroll_timer);
    }

    if (hdl->y_scroll_timer) {
        hdl->y_scroll_timer -= SGN(hdl->y_scroll_timer);
    }

    if (hdl->y_bump_timer) {
        hdl->y_bump_timer -= SGN(hdl->y_bump_timer);
    }

    if (hdl->x_bump_timer) {
        hdl->x_bump_timer -= SGN(hdl->x_bump_timer);
    }

    if ((hdl->b & (PLTF_PD_BTN_DL | PLTF_PD_BTN_DR)) &&
        !(hdl->bp & (PLTF_PD_BTN_DL | PLTF_PD_BTN_DR))) {
        i32 dx = (hdl->b & PLTF_PD_BTN_DL) ? -1 : +1;
        hdl->pos_x += dx;
        hdl->pos_x = (hdl->pos_x + 4) % 4;
        if (hdl->only_letters && hdl->pos_x == TI_PD_COL_SPECIAL) {
            hdl->pos_x += dx;
        }
        hdl->pos_x = (hdl->pos_x + 4) % 4;

        hdl->x_scroll_timer = 2 * dx;
        hdl->x_bump_timer   = 3 * dx;
        snd_play(dx == 1 ? SNDID_KB_SELECTION : SNDID_KB_SELECTION_REV,
                 1.f, 1.f);
    }

    if ((hdl->b & PLTF_PD_BTN_B) && !(hdl->bp & PLTF_PD_BTN_B)) {
        textinput_del_char(hdl->tinp);
        snd_play(SNDID_KB_KEY, 1.f, 1.f);
    }

    if (hdl->b & PLTF_PD_BTN_A) {
        hdl->a_hold_timer++;
    } else {
        hdl->a_hold_timer = 0;
    }

    if (hdl->a_hold_timer == 1 ||
        (TI_PD_SCROLL_DELAY <= hdl->a_hold_timer &&
         (hdl->a_hold_timer % TI_PD_A_SCROLL_MOD) == 0)) {
        if (hdl->pos_x == TI_PD_COL_UTIL) {
            switch (hdl->pos_y[TI_PD_COL_UTIL]) {
            case 0: // _
                textinput_add_char(hdl->tinp, ' ');
                break;
            case 1: // ok
                hdl->close_tick = TI_PD_CLOSE_TICK;
                break;
            case 2: // del (B)
                textinput_del_char(hdl->tinp);
                break;
            case 3: // cancel
                hdl->close_tick = TI_PD_CLOSE_TICK;
                hdl->tinp->n    = hdl->saved_n;
                mcpy(hdl->tinp->c, hdl->saved_c, NUM_TEXTINPUT_CHARS);
                break;
            }
        } else {
            char c = g_kbrd[hdl->pos_x][hdl->pos_y[hdl->pos_x]];
            textinput_add_char(hdl->tinp, c);
        }
        snd_play(SNDID_KB_KEY, 1.f, 1.f);
    }

    if (hdl->b & (PLTF_PD_BTN_DD | PLTF_PD_BTN_DU)) {
        hdl->y_hold_timer++;
    } else {
        hdl->y_hold_timer = 0;
    }

    i32 dy = inp_crank_click_turn_by(&hdl->crank_click, crankdt);
    if (hdl->y_hold_timer == 1 ||
        (TI_PD_SCROLL_DELAY <= hdl->y_hold_timer &&
         (hdl->y_hold_timer % TI_PD_Y_SCROLL_MOD) == 0)) { // scroll y
        dy += (hdl->b & PLTF_PD_BTN_DU) ? -1 : +1;
    }

    if (dy) {
        switch (hdl->pos_x) {
        case TI_PD_COL_UTIL: {
            i32 pos_y_new = hdl->pos_y[TI_PD_COL_UTIL] + dy;
            i32 pos_y1    = hdl->only_letters ? 1 : 0;
            if (!(pos_y1 <= pos_y_new && pos_y_new <= 3)) { // bump
                hdl->y_bump_timer = 3 * sgn_i32(dy);
                dy                = 0;
                snd_play(SNDID_KB_DENIAL, 1.f, 1.f);
            } else {
                hdl->pos_y[TI_PD_COL_UTIL] = pos_y_new;
            }
            break;
        }
        case TI_PD_COL_LOWER:
        case TI_PD_COL_UPPER: {
            hdl->pos_y[TI_PD_COL_UPPER] += dy + 26;
            hdl->pos_y[TI_PD_COL_UPPER] %= 26;
            hdl->pos_y[TI_PD_COL_LOWER] = hdl->pos_y[TI_PD_COL_UPPER];
            break;
        }
        case TI_PD_COL_SPECIAL: {
            hdl->pos_y[TI_PD_COL_SPECIAL] += dy + 42;
            hdl->pos_y[TI_PD_COL_SPECIAL] %= 42;
            break;
        }
        }

        if (dy != 0) {
            snd_play(SNDID_KB_CLICK, 1.f, 1.f);
            hdl->y_bump_timer   = 3;
            hdl->y_scroll_timer = TI_PD_Y_SCROLL_TIMER * sgn_i32(dy);
        }
    }
}

void textinput_pd_draw(textinput_handler_s *hdl)
{
    gfx_ctx_s ctx     = gfx_ctx_display();
    tex_s     tex     = asset_tex(TEXID_KEYBOARD);
    i32       tb_offs = textinput_draw_offs();
    i32       offs_x  = 400 - tb_offs;
    rec_i32   rfill   = {offs_x, 0, tb_offs, 240};
    gfx_rec_fill(ctx, rfill, GFX_COL_BLACK);

    i32 y_scroll = (hdl->y_scroll_timer * 36) / TI_PD_Y_SCROLL_TIMER;

    i32 col1 = hdl->only_letters ? 1 : 0;

    for (i32 n = col1; n < 4; n++) {
        texrec_s tr_util = {tex, 608, 32 * n, 64, 32};
        i32      offs_y  = (n - hdl->pos_y[TI_PD_COL_UTIL]) * TI_PD_SPACE_Y;
        if (hdl->pos_x == 3) {
            offs_y += y_scroll;
        }
        v2_i32 putil = {offs_x + 120 - col1 * 36,
                        offs_y + 94};
        gfx_spr(ctx, tr_util, putil, 0, SPR_MODE_WHITE);
    }

    for (i32 xx = col1; xx < 3; xx++) {
        i32 offs_x_glyph = offs_x + 12 + (xx - col1) * 36;
        i32 offs_y       = 104;

        if (hdl->pos_x == xx || ((hdl->pos_x == TI_PD_COL_LOWER &&
                                  xx == TI_PD_COL_UPPER) ||
                                 (hdl->pos_x == TI_PD_COL_UPPER &&
                                  xx == TI_PD_COL_LOWER))) {
            offs_y += y_scroll;
        }

        for (i32 n = -3; n <= +4; n++) {
            i32 gID = hdl->pos_y[xx] + n;
            switch (xx) {
            case TI_PD_COL_SPECIAL: gID = (gID + 42) % 42; break;
            case TI_PD_COL_UPPER:
            case TI_PD_COL_LOWER: gID = (gID + 26) % 26; break;
            }

            v2_i32   pglyph   = {offs_x_glyph, offs_y + n * TI_PD_SPACE_Y};
            char     c        = g_kbrd[xx][gID];
            i32      glyphx   = ((u32)c - 32) & 15;
            i32      glyphy   = ((u32)c - 32) >> 4;
            texrec_s tr_glyph = {tex, glyphx * 36, glyphy * 36, 36, 36};
            gfx_spr(ctx, tr_glyph, pglyph, 0, SPR_MODE_WHITE);
        }
    }

    rec_i32 rselect = {offs_x + 12 + (hdl->pos_x - col1) * 36,
                       240 / 2 - 20,
                       hdl->pos_x == 3 ? 50 : 36,
                       38};
    rselect.x += hdl->x_bump_timer;
    rselect.y += hdl->y_bump_timer;
    gfx_rec_rounded_fill(ctx, rselect, 2, PRIM_MODE_INV);
}
#endif

#ifdef PLTF_SDL
void textinput_sdl_close_input(void *ctx)
{
    textinput_deactivate();
}

void textinput_sdl_char_add(char c, void *ctx)
{
    textinput_handler_s *hdl = &g_textinput_handler;
    if (hdl->only_letters && !(('a' <= c && c <= 'z') ||
                               ('A' <= c && c <= 'Z')))
        return;
    textinput_add_char(hdl->tinp, c);
}

void textinput_sdl_char_del(void *ctx)
{
    textinput_handler_s *hdl = &g_textinput_handler;
    textinput_del_char(hdl->tinp);
}
#endif