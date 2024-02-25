// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "lighting.h"
#include "game.h"

static void light_do(tex_s sm, light_s light, wall_s *walls, int n_w);

void lighting_do(tex_s tex, light_s *lights, int n_l, wall_s *walls, int n_w)
{
    spm_push();

    tex_s shadowmap = tex_create_opaque(tex.w, tex.h, spm_allocator);
    tex_clr(shadowmap, TEX_CLR_BLACK);

    for (int n = 0; n < n_l; n++) {
        light_do(shadowmap, lights[n], walls, n_w);
    }

    u32 *tx = tex.px;
    u32 *sm = shadowmap.px;
    int  N  = tex.h * tex.wword;
    for (int n = 0; n < N; n++) {
        *tx++ &= *sm++;
    }

    spm_pop();
}

static void light_do(tex_s sm, light_s light, wall_s *walls, int n_w)
{
    spm_push();

    tex_s shadowmap_l = tex_create_opaque(sm.w, sm.h, spm_allocator);
    tex_clr(shadowmap_l, TEX_CLR_BLACK);

    gfx_ctx_s ctx  = gfx_ctx_default(shadowmap_l);
    gfx_ctx_s ctx1 = gfx_ctx_default(shadowmap_l);
    gfx_ctx_s ctx2 = gfx_ctx_default(shadowmap_l);
    ctx1.pat       = gfx_pattern_interpolate(2, 4);
    ctx2.pat       = gfx_pattern_interpolate(1, 4);

    int r   = 200;
    u32 rsq = r * r;

    gfx_cir_fill(ctx2, light.p, r * 2, PRIM_MODE_WHITE);
    gfx_cir_fill(ctx1, light.p, (r * 15) / 8, PRIM_MODE_WHITE);
    gfx_cir_fill(ctx, light.p, (r * 14) / 8, PRIM_MODE_WHITE);

    ctx.pat = gfx_pattern_interpolate(4, 4);

    for (int n = 0; n < n_w; n++) {
        wall_s w  = walls[n];
        v2_i32 da = v2_sub(w.a, light.p);
        v2_i32 db = v2_sub(w.b, light.p);

        if (rsq <= v2_lensq(da)) continue;
        if (rsq <= v2_lensq(db)) continue;

        da = v2_mul(da, 100);
        db = v2_mul(db, 100);

        v2_i32 poly[4] = {
            w.a,
            w.b,
            v2_add(w.b, db),
            v2_add(w.a, da)};
        gfx_poly_fill(ctx, poly, 4, PRIM_MODE_BLACK);
    }

    u32 *sp = sm.px;
    u32 *sl = shadowmap_l.px;
    int  N  = sm.h * sm.wword;
    for (int n = 0; n < N; n++) {
        *sp++ |= *sl++;
    }

    spm_pop();
}