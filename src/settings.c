// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "settings.h"
#include "app.h"
#include "pltf/pltf.h"

typedef struct {
    ALIGNAS(8)
    u32 version;
    u16 checksum;
    u8  unused[2];
} settings_header_s;

static_assert(sizeof(settings_header_s) == 8, "size settings header");

#define SETTINGS_FILE_NAME "settings.bin"

settings_s SETTINGS;

err32 settings_load(settings_s *s)
{
#if 1
    // TODO
    // just load and override default settings for dev purposes
    settings_default(s);
    return 0;
#endif
    void *f = pltf_file_open_r(SETTINGS_FILE_NAME);
    if (!f) return SETTINGS_ERR_OPEN;

    settings_header_s h   = {0};
    err32             res = 0;

    if (pltf_file_r_checked(f, &h, sizeof(settings_header_s))) {
        game_version_s v = game_version_decode(h.version);

        if (v.vmaj == 0) {
            res |= SETTINGS_ERR_VERSION;
        } else {
            if (pltf_file_r_checked(f, s, sizeof(settings_s))) {
                if (h.checksum != crc16(s, sizeof(settings_s))) {
                    res |= SETTINGS_ERR_CHECKSUM;
                }
            } else {
                res |= SETTINGS_ERR_RW;
            }
        }
    } else {
        res |= SETTINGS_ERR_RW;
        settings_default(s);
    }

    if (!pltf_file_close(f)) {
        res |= SETTINGS_ERR_CLOSE;
    }
    return res;
}

err32 settings_save(settings_s *s)
{
    void *f = pltf_file_open_w(SETTINGS_FILE_NAME);
    if (!f) return SETTINGS_ERR_OPEN;

    err32             res = 0;
    settings_header_s h   = {0};
    h.version             = GAME_VERSION;
    h.checksum            = crc16(s, sizeof(settings_s));

    if (!pltf_file_w_checked(f, &h, sizeof(settings_header_s)) ||
        !pltf_file_w_checked(f, s, sizeof(settings_s))) {
        res |= SETTINGS_ERR_RW;
    }

    if (pltf_file_close(f)) {
        res |= SETTINGS_ERR_CLOSE;
    }
    return res;
}

void settings_default(settings_s *s)
{
    mclr(s, sizeof(settings_s));
    s->mode       = SETTINGS_MODE_NORMAL;
    s->vol_mus    = SETTINGS_VOL_MAX;
    s->vol_sfx    = SETTINGS_VOL_MAX;
    s->swap_ticks = SETTINGS_SWAP_TICKS_NORMAL;
}

#define SETTINGS_TICKS_LERP    10
#define SETTINGS_VOL_MAX_NOTCH 8

i32 settings_actual_value_of(settings_m_s *s, i32 elID)
{
    settings_el_s *el = &s->settings_el[elID];
    switch (elID) {
    case SETTINGS_EL_VOL_MUS:
    case SETTINGS_EL_VOL_SFX:
        return ((256 * el->v_curr) / SETTINGS_VOL_MAX_NOTCH);
    case SETTINGS_EL_MODE:
        break;
    }
    return 0;
}

void settings_el_upd_range(settings_el_s *s, i32 ix, i32 vmax);        // range with min and max; slider
void settings_el_upd_options(settings_el_s *s, i32 ix, i32 n_options); // range; looping through options
void settings_el_upd_checkbox(settings_el_s *s, i32 ia);

void settings_enter(settings_m_s *s)
{
    s->active = 1;
}

void settings_update(settings_m_s *s)
{
    i32 ix = inp_btn_jp(INP_DL) ? -1 : (inp_btn_jp(INP_DR) ? +1 : 0);
    i32 iy = inp_btn_jp(INP_DU) ? -1 : (inp_btn_jp(INP_DD) ? +1 : 0);
    i32 ia = inp_btn_jp(INP_A);
    i32 ib = inp_btn_jp(INP_B);

    if (ib) {
        s->active = 0;
        return;
    }

    if (iy) {
        s->n_prev = s->n_curr;
        s->n_curr = clamp_i32((i32)s->n_curr + iy, 0, NUM_SETTINGS_EL - 1);
        if (s->n_curr != s->n_prev) {
            s->tick_lerp = 1;
        }
        return;
    }

    if (s->tick_lerp) {
        s->tick_lerp++;
        if (SETTINGS_TICKS_LERP <= s->tick_lerp) {
            s->n_prev    = s->n_curr;
            s->tick_lerp = 0;
        }
    }

    for (i32 i = 0; i < NUM_SETTINGS_EL; i++) {
        settings_el_s *el = &s->settings_el[i];
        if (el->tick) {
            el->tick++;
            if (SETTINGS_TICKS_LERP <= el->tick) {
                el->v_prev = el->v_curr;
                el->tick   = 0;
            }
        }
    }

    settings_el_s *el = &s->settings_el[s->n_curr];
    switch (s->n_curr) {
    case SETTINGS_EL_VOL_MUS:
    case SETTINGS_EL_VOL_SFX:
        settings_el_upd_range(el, ix, SETTINGS_VOL_MAX_NOTCH);
        break;
    case SETTINGS_EL_MODE:
        settings_el_upd_checkbox(el, ia ? ia : (ix != 0));
        break;
    }
}

static void settings_draw_label(gfx_ctx_s ctx, fnt_s f, v2_i32 p, const void *t)
{
    v2_i32 pos = {p.x + 20, p.y};
    fnt_draw_str(ctx, f, pos, t, SPR_MODE_BLACK);
}

static void settings_draw_checkbox(gfx_ctx_s ctx, v2_i32 p, settings_el_s *el)
{
    i32     sbox = 12;
    rec_i32 r1   = {p.x + 250 - sbox / 2, p.y + 8 - sbox / 2, sbox, sbox};
    rec_i32 r2   = {r1.x + 2, r1.y + 2, sbox - 4, sbox - 4};
    gfx_rec_fill(ctx, r1, PRIM_MODE_BLACK);
    gfx_rec_fill(ctx, r2, PRIM_MODE_WHITE);
    if (el->v_curr == 1) {
        rec_i32 r3 = {r2.x + 1, r2.y + 1, sbox - 6, sbox - 6};
        gfx_rec_fill(ctx, r3, PRIM_MODE_BLACK);
    }
}

static void settings_draw_slider(gfx_ctx_s ctx, v2_i32 p, settings_el_s *el, i32 vmax)
{
    i32       w    = 60;
    rec_i32   r1   = {p.x + 250, p.y, w, 10};
    gfx_ctx_s ctxh = ctx;
    ctxh.pat       = gfx_pattern_50();
    gfx_rec_fill(ctxh, r1, PRIM_MODE_BLACK_WHITE);

    rec_i32 r2 = {r1.x - 2 + (w * el->v_curr) / vmax, r1.y - 1, 4, 12};
    gfx_rec_fill(ctx, r2, PRIM_MODE_BLACK);
}

static void settings_draw_options(gfx_ctx_s ctx, v2_i32 p, settings_el_s *el,
                                  const void **options, i32 n_options)
{
}

void settings_draw(settings_m_s *s)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    fnt_s     fnt = asset_fnt(FNTID_SMALL);
    tex_clr(ctx.dst, GFX_COL_WHITE);

    v2_i32 pos = {0, 20};
    for (i32 i = 0; i < NUM_SETTINGS_EL; i++) {
        settings_el_s *el       = &s->settings_el[i];
        bool32         selected = i == s->n_curr;
        if (selected) {
            v2_i32 pselect = {pos.x + 6, pos.y + 8};
            gfx_cir_fill(ctx, pselect, 8, PRIM_MODE_BLACK);
        }

        switch (i) {
        case SETTINGS_EL_VOL_MUS:
            settings_draw_label(ctx, fnt, pos, "Vol Music");
            settings_draw_slider(ctx, pos, el, SETTINGS_VOL_MAX_NOTCH);
            break;
        case SETTINGS_EL_VOL_SFX:
            settings_draw_label(ctx, fnt, pos, "Vol SFX");
            settings_draw_slider(ctx, pos, el, SETTINGS_VOL_MAX_NOTCH);
            break;
        case SETTINGS_EL_MODE:
            settings_draw_label(ctx, fnt, pos, "Power saving (lower FPS)");
            settings_draw_checkbox(ctx, pos, el);
            break;
        }

        pos.y += 20;
    }
}

// range with min and max; slider
void settings_el_upd_range(settings_el_s *s, i32 ix, i32 vmax)
{
    s->v_prev = s->v_curr;
    s->v_curr = clamp_i32((i32)s->v_curr + ix, 0, vmax);
    if (s->v_curr != s->v_prev) {
        s->tick = 1;
    }
}

// range; looping through options
void settings_el_upd_options(settings_el_s *s, i32 ix, i32 n_options)
{
    s->v_prev = s->v_curr;
    s->v_curr = ((i32)s->v_curr + ix + n_options) % n_options;
    if (s->v_curr != s->v_prev) {
        s->tick = 1;
    }
}

void settings_el_upd_checkbox(settings_el_s *s, i32 ia)
{
    if (ia) {
        s->v_prev = s->v_curr;
        s->v_curr = 1 - s->v_curr;
        s->tick   = 1;
    }
}