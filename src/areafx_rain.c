// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#if 0
void areafx_rain_setup(g_s *g, areafx_rain_s *fx)
{
    fx->n_drops        = 100;
    fx->lightning_tick = 0;

    for (i32 k = 0; k < fx->n_drops; k++) {
        areafx_raindrop_s *drop = &fx->drops[k];
        drop->p.x               = rngr_i32(0, AREAFX_RAIN_W - 1) << 4;
        drop->p.y               = rngr_i32(0, AREAFX_RAIN_H - 1) << 4;
        drop->v.y               = rngr_i32(2000, 2800) >> 4;
        drop->v.x               = rngr_i32(100, 700) >> 4;
    }
}

void areafx_rain_update(g_s *g, areafx_rain_s *fx)
{
    if (fx->lightning_tick) {
        fx->lightning_tick--;
    }
    if (rngr_i32(0, 1000) <= 4) {
        fx->lightning_tick  = rngr_i32(6, 10);
        fx->lightning_twice = rngr_i32(0, 4) == 0;
    }

    for (i32 n = 0; n < fx->n_drops; n++) {
        areafx_raindrop_s *drop = &fx->drops[n];
        drop->p                 = v2_i16_add(drop->p, drop->v);
    }
}

void areafx_rain_draw(g_s *g, areafx_rain_s *fx, v2_i32 cam)
{
    gfx_ctx_s ctx     = gfx_ctx_display();
    gfx_ctx_s ctxdrop = ctx;
    ctxdrop.pat       = gfx_pattern_4x4(B4(0000),
                                        B4(1111),
                                        B4(0000),
                                        B4(1111));
    for (i32 n = 0; n < fx->n_drops; n++) {
        areafx_raindrop_s *drop = &fx->drops[n];
        v2_i32             p    = v2_i32_shr(v2_i32_from_i16(drop->p), 4);
        p                       = v2_i32_add(p, cam);
        p.x &= AREAFX_RAIN_W - 1;
        p.y &= AREAFX_RAIN_H - 1;
        p.x -= (AREAFX_RAIN_W - PLTF_DISPLAY_W) >> 1;
        p.y -= (AREAFX_RAIN_H - PLTF_DISPLAY_H) >> 1;

        rec_i32 rr1 = {p.x, p.y + 5, 3, 8};
        rec_i32 rr2 = {p.x, p.y, 2, 5};
        rec_i32 rr3 = {p.x, p.y + 1, 1, 6};
        gfx_rec_fill(ctx, rr1, PRIM_MODE_BLACK);
        gfx_rec_fill(ctx, rr2, PRIM_MODE_BLACK);
        gfx_rec_fill(ctxdrop, rr3, PRIM_MODE_WHITE);
    }
}

void areafx_rain_draw_lightning(g_s *g, areafx_rain_s *fx, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    if (fx->lightning_tick) {
        gfx_ctx_s ctx_light = ctx;

        if (5 < fx->lightning_tick) {
            ctx_light.pat = gfx_pattern_interpolate(1, 4);
        } else if (3 < fx->lightning_tick) {
            if (fx->lightning_twice) {
                ctx_light.pat = gfx_pattern_interpolate(0, 2);
            } else {
                ctx_light.pat = gfx_pattern_interpolate(1, 2);
            }
        } else {
            ctx_light.pat = gfx_pattern_interpolate(1, 8);
        }

        gfx_rec_fill(ctx_light, CINIT(rec_i32){0, 0, 400, 240}, PRIM_MODE_WHITE);
    }
}
#endif