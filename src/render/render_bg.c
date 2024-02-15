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

int water_render_height(game_s *g, int pixel_x)
{
    int p = pixel_x;
    int t = sys_tick();
    i32 y = (sin_q6((p << 2) + (t << 4) + 0x20) << 1) +
            (sin_q6((p << 4) - (t << 5) + 0x04) << 0) +
            (sin_q6((p << 5) + (t << 6) + 0x10) >> 2);
    return (y >> 6);
}

void render_bg(game_s *g, rec_i32 cam, bounds_2D_s bounds)
{
    const gfx_ctx_s ctx    = gfx_ctx_display();
    const v2_i32    camoff = {-cam.x, -cam.y};
    tex_s           dst    = ctx.dst;

    ocean_s *ocean = &g->ocean;
    ocean_calc_spans(g, cam);

    int clip_bot = min_i(ocean->y_max, dst.h);

    gfx_ctx_s     ctx_ocean     = ctx;
    gfx_pattern_s pattern_black = gfx_pattern_bayer_4x4(0);
    gfx_pattern_s pattern_white = gfx_pattern_bayer_4x4(16);
    switch (g->areaID) {
    case 0: {
        bg_fill_rows(dst, pattern_white, 0, clip_bot);
        break;
    }
    case 1: {
        bg_fill_rows(dst, pattern_black, 0, clip_bot);
        break;
    }
    }

    if (g->env_effects & ENVEFFECT_CLOUD) {
        enveffect_cloud_draw(ctx, &g->env_cloud, camoff);
    }

    switch (g->areaID) {
    case 0: {
        texrec_s tcave_1   = asset_texrec(TEXID_BG_MOUNTAINS, 0, 0, 1024, 256);
        texrec_s tcave_2   = asset_texrec(TEXID_BG_MOUNTAINS, 0, 256, 1024, 256);
        v2_i32   cavepos_1 = {(0 - cam.x * 4) / 16, 0};
        v2_i32   cavepos_2 = {(0 - cam.x * 2) / 16, 0};
        cavepos_1.x &= ~1; // reduce dither flickering
        cavepos_1.y &= ~1;
        cavepos_2.x &= ~1;
        cavepos_2.y &= ~1;

        gfx_ctx_s ctx_bg = gfx_ctx_clip_bot(ctx, clip_bot);
        gfx_spr(ctx_bg, tcave_1, cavepos_2, 0, 0);
        gfx_spr(ctx_bg, tcave_2, cavepos_1, 0, 0);
        break;
    }
    case 1: {
        texrec_s tcave_1   = asset_texrec(TEXID_BG_CAVE, 0, 0, 1024, 256);
        texrec_s tcave_2   = asset_texrec(TEXID_BG_CAVE, 0, 256, 1024, 256);
        v2_i32   cavepos_1 = {(0 - cam.x * 3) / 4, 20};
        v2_i32   cavepos_2 = {(0 - cam.x * 2) / 4, 0};
        cavepos_1.x &= ~1; // reduce dither flickering
        cavepos_1.y &= ~1;
        cavepos_2.x &= ~3;
        cavepos_2.y &= ~3;
        gfx_spr(ctx, tcave_2, cavepos_2, 0, 0);
        gfx_spr(ctx, tcave_1, cavepos_1, 0, 0);
        break;
    }
    }

    int      tick   = sys_tick();
    texrec_s twater = asset_texrec(TEXID_WATER_PRERENDER, 0, 0, 16, 16);
    for (int y = bounds.y1; y <= bounds.y2; y++) {
        for (int x = bounds.x1; x <= bounds.x2; x++) {
            int    i  = x + y * g->tiles_x;
            tile_s rt = g->tiles[i];
            if (!(rt.type & TILE_WATER_MASK)) continue;
            v2_i32 p = {(x << 4) + camoff.x, (y << 4) + camoff.y};
            int    j = i - g->tiles_x;
            if (0 <= j && !(g->tiles[j].type & TILE_WATER_MASK)) {
                twater.r.y = water_tile_get(x, y, tick) << 4;
                gfx_spr(ctx_ocean, twater, p, 0, 0);
            } else {
                gfx_rec_fill(ctx, (rec_i32){p.x, p.y, 16, 16}, PRIM_MODE_BLACK);
            }
        }
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

    tex_s t     = ctx.dst;
    int   y_max = clamp_i(oc->y_max, 0, t.h);

    for (int k = 0, x = 0; k < oc->n_spans; k++) {
        ocean_span_s sp = oc->spans[k];
        rec_i32      rf = {x, sp.y, sp.w, y_max - sp.y};

        gfx_rec_fill(ctx, rf, PRIM_MODE_BLACK);
        x += sp.w;
    }

    u32 *px = &((u32 *)t.px)[y_max * t.wword];
    int  N  = t.wword * (t.h - y_max);
    for (int n = 0; n < N; n++) {
        *px++ = 0;
    }
}