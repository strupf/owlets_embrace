// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "vfx_area.h"

void vfx_area_snow_setup(g_s *g)
{
    g->vfx_area_mem     = game_alloct(g, vfx_area_snow_s);
    vfx_area_snow_s *fx = (vfx_area_snow_s *)g->vfx_area_mem;
    mclr(fx, sizeof(vfx_area_snow_s));

    fx->n = 220;
    for (i32 n = 0; n < fx->n; n++) {
        vfx_area_snowflake_s *sf = &fx->snowflakes[n];
        sf->p_q4.x               = rngr_i32(0, VFX_AREA_SNOW_W - 1) << 4;
        sf->p_q4.y               = rngr_i32(0, VFX_AREA_SNOW_H - 1) << 4;
        sf->v_q4.x               = -rngr_i32(10, 160);
        sf->v_q4.y               = 10;
    }
    assert(fx->n <= VFX_AREA_SNOW_NUM_SNOWFLAKES);
}

void vfx_area_snow_update(g_s *g)
{
    vfx_area_snow_s *fx = (vfx_area_snow_s *)g->vfx_area_mem;

    for (i32 n = 0; n < fx->n; n++) {
        vfx_area_snowflake_s *sf = &fx->snowflakes[n];
        sf->p_q4                 = v2_i16_add(sf->p_q4, sf->v_q4);
        i32 vy                   = sf->v_q4.y + rngr_sym_i32(8);
        sf->v_q4.y               = clamp_i32(vy, -20, +40);
        i32 vx                   = sf->v_q4.x + rngr_sym_i32(4);
        sf->v_q4.x               = clamp_i32(vx, -180, 30);
    }
}

void vfx_area_snow_draw(g_s *g, v2_i32 cam)
{
    vfx_area_snow_s *fx  = (vfx_area_snow_s *)g->vfx_area_mem;
    gfx_ctx_s        ctx = gfx_ctx_display();

    for (i32 n = 0; n < fx->n; n++) {
        vfx_area_snowflake_s *sf = &fx->snowflakes[n];
        v2_i32                p  = v2_i32_shr(v2_i32_from_i16(sf->p_q4), 4);
        p                        = v2_i32_add(p, cam);
        p.x &= VFX_AREA_SNOW_W - 1;
        p.y &= VFX_AREA_SNOW_H - 1;
        p.x -= (VFX_AREA_SNOW_W - PLTF_DISPLAY_W) >> 1;
        p.y -= (VFX_AREA_SNOW_H - PLTF_DISPLAY_H) >> 1;
        i32     s = (8 * n) / fx->n + 1;
        rec_i32 r = {p.x, p.y, s, s};
        gfx_cir_fill(ctx, p, s, GFX_COL_WHITE);
    }
}
