// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "areafx.h"
#include "game.h"

void areafx_snow_setup(g_s *g, areafx_snow_s *fx)
{
    switch (fx->type) {
    case AREAFX_SNOW_NO_WIND: {
        fx->n = 140;
        for (i32 n = 0; n < fx->n; n++) {
            areafx_snowflake_s *sf = &fx->snowflakes[n];
            sf->p_q4.x             = rngr_i32(0, AREAFX_SNOW_W - 1) << 4;
            sf->p_q4.y             = rngr_i32(0, AREAFX_SNOW_H - 1) << 4;
            sf->v_q4.x             = -rngr_i32(20, 60);
            sf->v_q4.y             = 32;
        }
        break;
    }
    case AREAFX_SNOW_MODERATE: {
        for (i32 n = 0; n < fx->n; n++) {
            areafx_snowflake_s *sf = &fx->snowflakes[n];
        }
        break;
    }
    case AREAFX_SNOW_STORM: {
        for (i32 n = 0; n < fx->n; n++) {
            areafx_snowflake_s *sf = &fx->snowflakes[n];
        }
        break;
    }
    }
    assert(fx->n <= AREAFX_NUM_SNOWFLAKES);
}

void areafx_snow_update(g_s *g, areafx_snow_s *fx)
{
    for (i32 n = 0; n < fx->n; n++) {
        areafx_snowflake_s *sf = &fx->snowflakes[n];
        sf->p_q4               = v2_i16_add(sf->p_q4, sf->v_q4);

        switch (fx->type) {
        case AREAFX_SNOW_NO_WIND: {
            i32 vy     = sf->v_q4.y + rngr_sym_i32(4);
            sf->v_q4.y = clamp_i32(vy, -20, +60);
            break;
        }
        case AREAFX_SNOW_MODERATE: {
            break;
        }
        case AREAFX_SNOW_STORM: {
            break;
        }
        }
    }
}

void areafx_snow_draw(g_s *g, areafx_snow_s *fx, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    for (i32 n = 0; n < fx->n; n++) {
        areafx_snowflake_s *sf = &fx->snowflakes[n];
        v2_i32              p  = v2_shr(v2_i32_from_i16(sf->p_q4), 4);
        p                      = v2_add(p, cam);
        p.x &= AREAFX_SNOW_W - 1;
        p.y &= AREAFX_SNOW_H - 1;
        p.x -= (AREAFX_SNOW_W - PLTF_DISPLAY_W) >> 1;
        p.y -= (AREAFX_SNOW_H - PLTF_DISPLAY_H) >> 1;
        i32     s = (4 * n) / fx->n;
        rec_i32 r = {p.x, p.y, s, s};
        gfx_rec_fill(ctx, r, GFX_COL_WHITE);
    }
}
