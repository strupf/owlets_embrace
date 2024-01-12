// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

void render_bg(game_s *g, rec_i32 cam)
{
    const gfx_ctx_s ctx    = gfx_ctx_display();
    const v2_i32    camoff = {-cam.x, -cam.y};

    gfx_ctx_s ctx_near = ctx;
    gfx_ctx_s ctx_far  = ctx;
    ctx_near.pat       = gfx_pattern_4x4(B4(1000),
                                         B4(0000),
                                         B4(0010),
                                         B4(0000));
    ctx_far.pat        = gfx_pattern_4x4(B4(1000),
                                         B4(0000),
                                         B4(0000),
                                         B4(0000));

    texrec_s tbackground = asset_texrec(TEXID_BG_ART, 0, 0, 128, 128);

    for (int l = 3; l >= 2; l--) {
        gfx_ctx_s ct = l == 2 ? ctx_far : ctx_near;
        for (int i = 0; i < 10; i++) {
            v2_i32 pos = {i * 128 - cam.x * l / 4, 50};
            gfx_spr(ct, tbackground, pos, 0, 0);
        }
    }

    ocean_s *ocean = &g->ocean;
    ocean_calc_spans(g, cam);
    ocean_draw_bg(g, asset_tex(0), camoff);
}
