// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

void render_bg(game_s *g, rec_i32 cam)
{
    const gfx_ctx_s ctx    = gfx_ctx_display();
    const v2_i32    camoff = {-cam.x, -cam.y};
    tex_s           dst    = ctx.dst;

    ocean_s *ocean = &g->ocean;
    ocean_calc_spans(g, cam);

    int clip_y1 = 200;
    int clip_y2 = min_i(ocean->y_max, dst.h);
    int clip_yy = min_i(clip_y1, clip_y2);

    gfx_ctx_s ctx_ocean = gfx_ctx_clip_top(ctx, 100);
#if 0
    gfx_ctx_s ctx_near  = gfx_ctx_clip_bot(ctx, clip_yy - 1);
    gfx_ctx_s ctx_far   = gfx_ctx_clip_bot(ctx, clip_yy - 1);

    ctx_near.pat = gfx_pattern_4x4(B4(1000),
                                   B4(0000),
                                   B4(0010),
                                   B4(0000));
    ctx_far.pat  = gfx_pattern_4x4(B4(1000),
                                   B4(0000),
                                   B4(0000),
                                   B4(0000));

    for (int y = 0; y < clip_yy; y++) {
        for (int x = 0; x < dst.wword; x++) {
            ((u32 *)dst.px)[x + y * dst.wword] = 0xFFFFFFFFU;
        }
    }

    for (int y = clip_yy; y < clip_y2; y++) {
        u32 p = ~ctx_near.pat.p[y & 3];
        for (int x = 0; x < dst.wword; x++) {
            ((u32 *)dst.px)[x + y * dst.wword] = p;
        }
    }
#else
    gfx_pattern_s pattern_fill = gfx_pattern_4x4(B4(0000),
                                                 B4(0000),
                                                 B4(0000),
                                                 B4(0000));
    for (int y = 0; y < dst.h; y++) {
        const u32 p  = pattern_fill.p[y & 3];
        u32      *px = &((u32 *)dst.px)[y * dst.wword];
        for (int x = 0; x < dst.wword; x++) {
            *px++ = p;
        }
    }
#endif
    texrec_s tcave_1   = asset_texrec(TEXID_BG_CAVE, 0, 0, 1024, 256);
    texrec_s tcave_2   = asset_texrec(TEXID_BG_CAVE, 0, 256, 1024, 256);
    v2_i32   cavepos_1 = {(0 - cam.x * 3) / 4, 20};
    v2_i32   cavepos_2 = {(0 - cam.x * 2) / 4, 0};
    if (sys_reduced_flicker()) {
        cavepos_1.x &= ~1;
        cavepos_1.y &= ~1;
        cavepos_2.x &= ~3;
        cavepos_2.y &= ~3;
    }
    gfx_spr_cpy_display(ctx, tcave_2, cavepos_2);
    gfx_spr_cpy_display(ctx, tcave_1, cavepos_1);

#if 0
    texrec_s tmountain = asset_texrec(TEXID_BG_ART, 0, 128, 256, 128);
    gfx_spr(ctx, tmountain, (v2_i32){0, 50}, 0, 0);

    texrec_s tbackground = asset_texrec(TEXID_BG_ART, 0, 0, 128, 128);

    for (int l = 3; l >= 3; l--) {
        gfx_ctx_s ct = ctx;
        for (int i = 0; i < 10; i++) {
            v2_i32 pos = {100 + i * 256 - cam.x * l / 4, 80};
            gfx_spr(ct, tbackground, pos, 0, 0);
        }
    }
#endif

    ocean_draw_bg(ctx_ocean, g, camoff);
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