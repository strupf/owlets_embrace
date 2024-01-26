// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

static void bg_fill_rows(tex_s dst, gfx_pattern_s pat, int y1, int y2)
{
    u32 *px = &((u32 *)dst.px)[y1 * dst.wword];
    for (int y = y1; y < y2; y++) {
        const u32 p = pat.p[y & 3];
        for (int x = 0; x < dst.wword; x++) {
            *px++ = p;
        }
    }
}

void render_bg(game_s *g, rec_i32 cam)
{
    const gfx_ctx_s ctx    = gfx_ctx_display();
    const v2_i32    camoff = {-cam.x, -cam.y};
    tex_s           dst    = ctx.dst;

    ocean_s *ocean = &g->ocean;
    ocean_calc_spans(g, cam);

    int clip_bot = min_i(ocean->y_max, dst.h);

    gfx_ctx_s     ctx_ocean     = gfx_ctx_clip_top(ctx, 100);
    gfx_pattern_s pattern_black = gfx_pattern_bayer_4x4(0);
    gfx_pattern_s pattern_white = gfx_pattern_bayer_4x4(16);
#if 1
    bg_fill_rows(dst, pattern_white, 0, clip_bot);
    texrec_s tcave_1   = asset_texrec(TEXID_BG_MOUNTAINS, 0, 0, 1024, 256);
    texrec_s tcave_2   = asset_texrec(TEXID_BG_MOUNTAINS, 0, 256, 1024, 256);
    v2_i32   cavepos_1 = {(0 - cam.x * 3) / 4, -20};
    v2_i32   cavepos_2 = {(0 - cam.x * 2) / 4, 0};
    cavepos_1.x &= ~1; // reduce dither flickering
    cavepos_1.y &= ~1;
    cavepos_2.x &= ~3;
    cavepos_2.y &= ~3;

    gfx_ctx_s ctx_bg = gfx_ctx_clip_bot(ctx, clip_bot);
    gfx_spr_cpy_display(ctx_bg, tcave_1, cavepos_1);
    gfx_spr_cpy_display(ctx_bg, tcave_2, cavepos_2);
#elif 0
    gfx_pattern_s pattern_1 = gfx_pattern_bayer_4x4(16);
    bg_fill_rows(dst, pattern_1, 0, clip_bot);
    texrec_s tcave_1   = asset_texrec(TEXID_CLOUDS, 0, 0, 512, 256);
    v2_i32   cavepos_1 = {0, 0};
    gfx_spr_cpy_display(ctx, tcave_1, cavepos_1);
#else
    bg_fill_rows(dst, pattern_black, 0, clip_bot);
    texrec_s tcave_1   = asset_texrec(TEXID_BG_CAVE, 0, 0, 1024, 256);
    texrec_s tcave_2   = asset_texrec(TEXID_BG_CAVE, 0, 256, 1024, 256);
    v2_i32   cavepos_1 = {(0 - cam.x * 3) / 4, 20};
    v2_i32   cavepos_2 = {(0 - cam.x * 2) / 4, 0};
    cavepos_1.x &= ~1; // reduce dither flickering
    cavepos_1.y &= ~1;
    cavepos_2.x &= ~3;
    cavepos_2.y &= ~3;
    gfx_spr_cpy_display(ctx, tcave_2, cavepos_2);
    gfx_spr_cpy_display(ctx, tcave_1, cavepos_1);
#endif

    if (g->env_effects & ENVEFFECT_CLOUD) {
        enveffect_cloud_draw(ctx, &g->env_cloud, camoff);
    }

    ocean_draw_bg(ctx_ocean, g, camoff);

#if 0
    for (int y = clip_yy; y < dst.h; y++) {
        for (int x = 0; x < dst.wword; x++) {
            ((u32 *)dst.px)[x + y * dst.wword] = 0xFFFFFFFFU;
        }
    }
#endif
}

void ocean_draw_bg(gfx_ctx_s ctx, game_s *g, v2_i32 camoff)
{
    ocean_s *oc = &g->ocean;
    if (!oc->active) return;

    tex_s t = ctx.dst;

#define HORIZONT_X     2000                // distance horizont from screen plane
#define HORIZONT_X_EYE 600                 // distance eye from screen plane
#define HORIZONT_Y_EYE (SYS_DISPLAY_H / 2) // height eye on screen plane (center)

    int y_max = clamp_i(oc->y_max, 0, t.h);

    for (int k = 0, x = 0; k < oc->n_spans; k++) {
        ocean_span_s sp = oc->spans[k];
        rec_i32      rf = {x, sp.y, sp.w, y_max - sp.y};

        gfx_rec_fill_display(ctx, rf, PRIM_MODE_BLACK);

#if 0
        // calc height of horizont based on thales theorem
        int y_horizont = ((sp.y - HORIZONT_Y_EYE) * HORIZONT_X) /
                         (HORIZONT_X + HORIZONT_X_EYE);
        for (int i = 1; i <= 8; i++) {
            int       yy   = sp.y - ((y_horizont * i) >> 3);
            rec_i32   rl   = {x, yy, sp.w, 1};
            gfx_ctx_s ctxl = ctx;
            ctxl.pat       = gfx_pattern_interpolate(8 - i, 8);
            gfx_rec_fill_display(ctxl, rl, PRIM_MODE_BLACK);
        }
#endif
        x += sp.w;
    }

    { // fill "static" bottom section
        u32 *px = &((u32 *)t.px)[y_max * t.wword];
        int  N  = t.wword * (t.h - y_max);
        for (int n = 0; n < N; n++) {
            *px++ = 0;
        }
    }
}