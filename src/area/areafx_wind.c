// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "areafx.h"
#include "game.h"

void areafx_wind_setup(g_s *g, areafx_wind_s *fx)
{
}

void areafx_wind_update(g_s *g, areafx_wind_s *fx)
{
    // UPDATE PARTICLES
    // traverse backwards to avoid weird removal while iterating
    for (i32 n = fx->n - 1; 0 <= n; n--) {
        areafx_windpt_s *p = &fx->p[n];
        if (p->p_q8.x < 0 || AREAFX_WIND_SIZEY < (p->p_q8.x >> 8)) {
            fx->p[n] = fx->p[--fx->n];
            continue;
        }

        p->circcooldown--;
        if (p->circcooldown <= 0 && rng_u32() < 0x4000000U) { // enter wind circle animation
            p->ticks        = rngr_u32(12, 15);
            p->circticks    = p->ticks;
            p->circc.x      = p->p_q8.x;
            p->circc.y      = p->p_q8.y - AREAFX_WIND_R;
            p->circcooldown = 50;
        }

        if (0 < p->circticks) { // run through circle but keep slowly moving forward
            i32 a     = ((p->ticks - p->circticks) << 18) / p->ticks;
            p->p_q8.x = p->circc.x + ((sin_q16(a) * AREAFX_WIND_R) >> 16);
            p->p_q8.y = p->circc.y + ((cos_q16(a) * AREAFX_WIND_R) >> 16);
            p->circc.x += 200;
            p->circticks--;
        } else {
            p->v_q8.y += rngr_sym_i32(60);
            p->v_q8.y = clamp_sym_i32(p->v_q8.y, 400);
            p->p_q8   = v2_i32_add(p->p_q8, p->v_q8);
        }

        // circular buffer of positions
        p->pos_q8[p->i] = p->p_q8;
        p->i            = (p->i + 1) & (AREAFX_WINDPT_N - 1);
    }

    // SPAWN PARTICLES
    if (fx->n < AREAFX_WINDPT && rng_u32() <= 0x20000000U) {
        areafx_windpt_s *p = &fx->p[fx->n++];
        mclr(p, sizeof(areafx_windpt_s));
        p->p_q8.y       = rngr_i32(0, AREAFX_WIND_SIZEY << 8);
        p->v_q8.x       = rngr_i32(2500, 5000);
        p->circcooldown = 10;
        for (i32 i = 0; i < AREAFX_WINDPT_N; i++)
            p->pos_q8[i] = p->p_q8;
    }
}

void areafx_wind_draw(g_s *g, areafx_wind_s *fx, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();

    for (i32 n = 0; n < fx->n; n++) {
        areafx_windpt_s p = fx->p[n];

        v2_i32    p1 = v2_i32_shr(p.pos_q8[p.i], 8);
        const i32 y1 = p1.y;
        const i32 h1 = ((y1 + cam.y) % AREAFX_WIND_SIZEY + AREAFX_WIND_SIZEY) % AREAFX_WIND_SIZEY;
        p1.y         = h1;

        for (i32 i = 1; i < AREAFX_WINDPT_N; i++) {
            i32    k    = (p.i + i) & (AREAFX_WINDPT_N - 1);
            i32    size = 1;
            v2_i32 p2   = v2_i32_shr(p.pos_q8[k], 8);
            p2.y        = h1 + (p2.y - y1);
            if (i < (AREAFX_WINDPT_N * 1) / 2) {
                ctx.pat = gfx_pattern_50();
                size    = 1;
            } else if (i < (AREAFX_WINDPT_N * 3) / 4) {
                ctx.pat = gfx_pattern_75();
                size    = 1;
            } else {
                ctx.pat = gfx_pattern_100();
            }
            gfx_lin_thick(ctx, p1, p2, GFX_COL_BLACK, size);
            p1 = p2;
        }
    }
}