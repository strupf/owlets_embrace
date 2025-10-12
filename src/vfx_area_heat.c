// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void vfx_area_heat_setup(g_s *g)
{
    g->vfx_area_mem     = game_alloc_roomt(g, vfx_area_heat_s);
    vfx_area_heat_s *fx = (vfx_area_heat_s *)g->vfx_area_mem;
}

void vfx_area_heat_update(g_s *g)
{
    vfx_area_heat_s *fx = (vfx_area_heat_s *)g->vfx_area_mem;
    fx->tick += 1500 + 200;
}

void vfx_area_heat_draw(g_s *g, v2_i32 cam)
{
    vfx_area_heat_s *fx = (vfx_area_heat_s *)g->vfx_area_mem;
    tex_s            t  = asset_tex(0);

    // shift scanlines left and right
    for (i32 y = 0; y < PLTF_DISPLAY_H; y++) {
        i32 a = (sin_q16(fx->tick + y * 3000) * 2) / 65536;
        a &= ~1;

        if (0 < a) { // shift scanline right
            i32  sh1 = +a;
            i32  sh2 = 32 - a;
            u32 *p   = &t.px[t.wword - 1 + y * t.wword];
            for (i32 x = 1; x < PLTF_DISPLAY_WWORDS; x++, p--) {
                *p = bswap32((bswap32(*(p + 0)) >> sh1) |
                             (bswap32(*(p - 1)) << sh2));
            }
            u32 pt    = bswap32(*(p + 0));
            u32 p_rem = pt >> sh1;
            if (pt & ((u32)1 << 31)) {
                p_rem |= 0xFFFFFFFF << sh2;
            }
            *p = bswap32(p_rem);
        } else if (a < 0) { // shift scanline left
            i32  sh1 = -a;
            i32  sh2 = 32 + a;
            u32 *p   = &t.px[0 + y * t.wword];
            for (i32 x = 1; x < PLTF_DISPLAY_WWORDS; x++, p++) {
                *p = bswap32((bswap32(*(p + 0)) << sh1) |
                             (bswap32(*(p + 1)) >> sh2));
            }
            // have to mask here because the screen area stop halfway
            // throughout the word
            u32 pt    = bswap32(*(p + 0)) & 0xFFFF0000;
            u32 p_rem = pt << sh1;
            if (pt & ((u32)1 << 16)) {
                p_rem |= 0xFFFFFFFF >> (sh2 - 16);
            }
            *p = bswap32(p_rem);
        }
    }
}