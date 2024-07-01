// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

void foreground_props_draw(game_s *g, v2_i32 cam)
{
    gfx_ctx_s ctx  = gfx_ctx_display();
    gfx_ctx_s ctxp = ctx;
    ctxp.pat       = gfx_pattern_2x2(B4(0010), B4(0011));

    for (i32 n = 0; n < g->n_foreground_props; n++) {
        foreground_prop_s *fp = &g->foreground_props[n];
        v2_i32             p  = v2_i32_from_i16(fp->pos);
        p                     = parallax_offs(cam, p, 12, 12);
        p                     = v2_add(p, cam);
        texrec_s tr1          = fp->tr;
        texrec_s tr2          = fp->tr;
        tr2.r.x += tr1.r.w;
        gfx_spr(ctx, tr1, p, 0, SPR_MODE_WHITE);
        gfx_spr(ctxp, tr1, p, 0, SPR_MODE_BLACK);
        gfx_spr(ctx, tr2, p, 0, 0);
    }
}