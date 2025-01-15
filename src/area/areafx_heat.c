// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "areafx.h"
#include "game.h"

void areafx_heat_setup(g_s *g, areafx_heat_s *fx)
{
}

void areafx_heat_update(g_s *g, areafx_heat_s *fx)
{
    fx->tick += 1500;
}

void areafx_heat_draw(g_s *g, areafx_heat_s *fx, v2_i32 cam)
{
    tex_s t = asset_tex(0);

    // shift scanlines left and right
    for (i32 y = 0; y < PLTF_DISPLAY_H; y++) {
        i32 a = (sin_q16(fx->tick + y * 3000) * 2) / 65536;

        if (0 < a) {
            i32  sh1 = +a;
            i32  sh2 = 32 - a;
            u32 *p   = &t.px[t.wword - 1 + y * t.wword];
            for (i32 x = 1; x < PLTF_DISPLAY_WWORDS; x++, p--) {
                *p = bswap32((bswap32(*(p + 0)) >> sh1) |
                             (bswap32(*(p - 1)) << sh2));
            }
            *p = bswap32(bswap32(*(p + 0)) >> sh1);
        } else if (a < 0) {
            i32  sh1 = -a;
            i32  sh2 = 32 + a;
            u32 *p   = &t.px[0 + y * t.wword];
            for (i32 x = 1; x < PLTF_DISPLAY_WWORDS; x++, p++) {
                *p = bswap32((bswap32(*(p + 0)) << sh1) |
                             (bswap32(*(p + 1)) >> sh2));
            }
            *p = bswap32(bswap32(*(p + 0)) << sh1);
        }
    }
}