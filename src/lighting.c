// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "lighting.h"
#include "game.h"

static u16 g_lig_sqrt[256];

void light_refresh(game_s *g, l_light_s *l);
void light_merge(lighting_s *lig, l_light_s *l, v2_i32 cam);

void lighting_init(lighting_s *lig)
{
    lig->merged = tex_create_opaque(400, 240, asset_allocator);
    for (i32 n = 0; n < 256; n++) {
        g_lig_sqrt[n] = sqrt_i32(n);
    }
}

void lighting_refresh(game_s *g, lighting_s *lig)
{
    for (i32 n = 0; n < lig->n_lights; n++) {
        light_refresh(g, &lig->lights[n]);
    }
}

void lighting_render(lighting_s *lig, v2_i32 cam)
{
    mset(lig->l, 0, sizeof(lig->l));
    for (i32 n = 0; n < lig->n_lights; n++) {
        light_merge(lig, &lig->lights[n], cam);
    }

    gfx_ctx_s ctx = gfx_ctx_default(lig->merged);
    tex_clr(lig->merged, GFX_COL_WHITE);
    i32 cx = (cam.x & 0) - 8;
    i32 cy = (cam.y & 0) - 8;
    for (i32 y = 1; y < (256 >> 3) - 1; y++) {
        for (i32 x = 1; x < (416 >> 3) - 1; x++) {
            i32           ll  = min_i32(lig->l[x + y * (416 >> 3)], 15);
            gfx_pattern_s pat = gfx_pattern_bayer_4x4(((16 - ll) * GFX_PATTERN_NUM) >> 4);
            i32           xx  = (((x << 3) - 8) >> 3);
            for (i32 ii = 0; ii < 8; ii++) {
                i32 yy = (y << 3) - 8 + ii;
                ((u8 *)lig->merged.px)[yy * 52 + xx] &= ~pat.p[ii];
            }
            // gfx_rec_fill(ctx2, (rec_i32){(x << 3) + cx, (y << 3) + cy, 8, 8}, PRIM_MODE_BLACK);
        }
    }
}

void lighting_apply_tex(lighting_s *lig)
{
    u32 *dp = &asset_tex(0).px[0];
    u32 *sp = &lig->merged.px[0];
    for (i32 n = 0; n < 240 * 13; n++) {
        *dp++ &= *sp++;
    }
}

void light_ray(game_s *g, l_light_s *lig, i32 ls, i32 w, v2_i32 p0, v2_i32 p1,
               i32 dx, i32 dy, i32 sx, i32 sy)
{
    i32 e     = dx + dy;
    i32 x     = p0.x;
    i32 y     = p0.y;
    i32 steps = 0;
    i32 cx    = lig->p.x - (p0.x << 3);
    i32 cy    = lig->p.y - (p0.y << 3);
    i32 st    = 15;
    i32 l     = ls * st;
    i32 u     = st;
    i32 v     = st;

    while (ls <= l) {
        u8 *lm = &lig->l[x + y * w];
        *lm    = max_i32(*lm, (u32)l / (u32)ls);

        if (steps) {
            l -= 128 * st;
            if ((--steps) <= 0) break;
        } else {
            rec_i32 r  = {cx + (x << 3), cy + (y << 3), 8, 8};
            bool32  px = map_traversable(g, r);
            if (!px) {
                steps = 4;
            }
        }

        i32 e2 = e << 1;
        if (e2 >= dy) {
            e += dy;
            x += sx;
            l -= u;
            u += st << 1;
        }
        if (e2 <= dx) {
            e += dx;
            y += sy;
            l -= v;
            v += st << 1;
        }
    }
}

void light_refresh(game_s *g, l_light_s *l)
{
    i32    rr = l->r >> 3;
    i32    mm = rr + 2;
    i32    w  = mm * 2 + 1;
    i32    ls = (l->r * l->r) >> (3 * 2);
    v2_i32 pm = {mm, mm};
    l->mm     = mm;
    l->w      = w;
    mset(l->l, 0, sizeof(u8) * w * w);
#if 1
    for (i32 y = 0; y < w; y++) {
        v2_i32 pl = {0, y};
        v2_i32 pr = {w - 1, y};
        light_ray(g, l, ls, w, pm, pl, mm, -abs_i32(y - mm), -1, mm < y ? +1 : -1);
        light_ray(g, l, ls, w, pm, pr, mm, -abs_i32(y - mm), +1, mm < y ? +1 : -1);
    }
    for (i32 x = 0; x < w; x++) {
        v2_i32 pu = {x, 0};
        v2_i32 pd = {x, w - 1};
        light_ray(g, l, ls, w, pm, pu, abs_i32(x - mm), -mm, mm < x ? +1 : -1, -1);
        light_ray(g, l, ls, w, pm, pd, abs_i32(x - mm), -mm, mm < x ? +1 : -1, +1);
    }
#else
    for (i32 y = 0; y < w; y++) {
        v2_i32 pl = {0, y};
        v2_i32 pr = {w - 1, y};
        light_ray(g, l, ls, w, pm, pl);
        light_ray(g, l, ls, w, pm, pr);
    }
    for (i32 x = 0; x < w; x++) {
        v2_i32 pu = {x, 0};
        v2_i32 pd = {x, w - 1};
        light_ray(g, l, ls, w, pm, pu);
        light_ray(g, l, ls, w, pm, pd);
    }
#endif
}

void light_merge(lighting_s *lig, l_light_s *l, v2_i32 cam)
{
    i32     mm = l->mm;
    i32     w  = l->w;
    rec_i32 r1 = {(l->p.x >> 3) + (cam.x >> 3) - mm + 1,
                  (l->p.y >> 3) + (cam.y >> 3) - mm + 1, w - 2, w - 2};
    rec_i32 r2 = {0, 0, (416 >> 3), (256 >> 3)};
    rec_i32 ri;
    if (!intersect_rec(r1, r2, &ri)) return;
    i32 px1 = ri.x;
    i32 py1 = ri.y;
    i32 px2 = ri.x + ri.w;
    i32 py2 = ri.y + ri.h;
    for (i32 y = py1; y < py2; y++) {
        i32 x           = px1;
        u8 *restrict l1 = &lig->l[x + y * (416 >> 3)];
        u8 *restrict l2 = &l->l[(x - r1.x) + (y - r1.y) * w];
        for (; x < px2 - 4; x += 4, l1 += 4, l2 += 4) {
            // vec32 v1 = vec32_ldu(l2);
            // vec32 v2 = vec32_ldu(l1);
            // vec32 v3 = u8x4_add(v1, v2);
            // vec32_stu(v3, l1);
        }
        for (; x < px2; x++, l1++, l2++) {
            *l1 = *l1 + *l2;
        }
    }
}

void lighting_shadowcast(game_s *g, lighting_s *lig, v2_i32 cam)
{
    gfx_ctx_s ctx_sm = gfx_ctx_default(lig->merged);
    tex_clr(ctx_sm.dst, GFX_COL_BLACK);

    spm_push();
    tex_s     tex  = tex_create_opaque(400, 240, spm_allocator);
    tex_s     tex2 = tex_create_opaque(400, 240, spm_allocator);
    gfx_ctx_s ctxl = gfx_ctx_default(tex);
    for (i32 n = 0; n < 1; n++) {

        tex_clr(tex, GFX_COL_BLACK);

        l_light_s *l   = &lig->lights[n];
        u32        rsq = l->r * l->r;

        v2_i32    pp    = v2_add(l->p, cam);
        gfx_ctx_s ctxl1 = ctxl;
        gfx_ctx_s ctxl2 = ctxl;
        // ctxl1.pat       = gfx_pattern_interpolate(14, 16);
        // ctxl2.pat       = gfx_pattern_interpolate(10, 16);
        gfx_cir_fill(ctxl, pp, (l->r * 2 * 8) / 8, GFX_COL_WHITE);
        // gfx_cir_fill(ctxl1, pp, (l->r * 2 * 6) / 8, GFX_COL_WHITE);
        // gfx_cir_fill(ctxl, pp, (l->r * 2 * 4) / 8, GFX_COL_WHITE);

        rec_i32 rlig = {l->p.x - l->r - 1,
                        l->p.y - l->r - 1,
                        l->r * 2 + 1,
                        l->r * 2 + 1};

        tile_map_bounds_s bounds = tile_map_bounds_rec(g, rlig);
        for (i32 yy = bounds.y1; yy <= bounds.y2; yy++) {
            for (i32 xx = bounds.x1; xx <= bounds.x2; xx++) {
                i32 tile = g->tiles[xx + yy * g->tiles_x].collision;
                if (!TILE_IS_SHAPE(tile)) continue;
                v2_i32         tpos = {xx << 4, yy << 4};
                tile_corners_s corn = g_tile_corners[tile];
                for (i32 n = 0; n < corn.n; n++) {
                    v2_i32 a = v2_add(v2_i32_from_v2_i8(corn.c[n]), tpos);
                    v2_i32 b = v2_add(v2_i32_from_v2_i8(corn.c[(n + 1) % corn.n]), tpos);

                    v2_i32 da = v2_sub(a, l->p);
                    v2_i32 db = v2_sub(b, l->p);

                    if (rsq <= v2_lensq(da) && rsq <= v2_lensq(db)) continue;

                    da = v2_shl(da, 8);
                    db = v2_shl(db, 8);
                    a  = v2_add(a, cam);
                    b  = v2_add(b, cam);

                    v2_i32 poly[4] = {
                        a,
                        b,
                        v2_add(b, db),
                        v2_add(a, da)};

                    gfx_poly_fill(ctxl, poly, 4, GFX_COL_BLACK);
                }
            }
        }

        for (obj_each(g, o)) {
            if (o->mass == 0) continue;
            tile_corners_s corn = {0};
            corn.n              = 4;
            corn.c[1]           = (v2_i8){(i8)o->w, 0};
            corn.c[2]           = (v2_i8){(i8)o->w, (i8)o->h};
            corn.c[3]           = (v2_i8){0, (i8)o->h};
            v2_i32 tpos         = o->pos;
            for (i32 n = 0; n < corn.n; n++) {
                v2_i32 a = v2_add(v2_i32_from_v2_i8(corn.c[n]), tpos);
                v2_i32 b = v2_add(v2_i32_from_v2_i8(corn.c[(n + 1) % corn.n]), tpos);

                v2_i32 da = v2_sub(a, l->p);
                v2_i32 db = v2_sub(b, l->p);

                // if (rsq <= v2_lensq(da) && rsq <= v2_lensq(db)) continue;

                da = v2_shl(da, 8);
                db = v2_shl(db, 8);
                a  = v2_add(a, cam);
                b  = v2_add(b, cam);

                v2_i32 poly[4] = {
                    a,
                    b,
                    v2_add(b, db),
                    v2_add(a, da)};

                gfx_poly_fill(ctxl, poly, 4, GFX_COL_BLACK);
            }
        }

        u32 *p1 = tex.px;
        u32 *p2 = tex2.px;

        for (i32 k = 0; k < 10; k++) {
            for (i32 y = 0; y < 240; y++) {
                for (i32 x = 0; x < 13; x++) {
                    i32 i  = x + y * tex2.wword;
                    u32 b2 = x > 0 ? (bswap32(p1[i - 1]) << 31) : 0;
                    u32 b1 = x < 12 ? (bswap32(p1[i + 1]) >> 31) : 0;
                    u32 a1 = bswap32((bswap32(p1[i]) << 1) | b1);
                    u32 a2 = bswap32((bswap32(p1[i]) >> 1) | b2);
                    u32 a3 = y < 240 - 1 ? p1[i + 13] : 0;
                    u32 a4 = y > 0 ? p1[i - 13] : 0;
                    p2[i]  = p1[i] | a1 | a2 | a3 | a4;
                }
            }
            u32 *tp = p1;
            p1      = p2;
            p2      = tp;
        }

        texrec_s tr  = {tex, {0, 0, tex.w, tex.h}};
        v2_i32   spp = {0, 0};
        gfx_spr(ctx_sm, tr, spp, 0, SPR_MODE_WHITE_ONLY);
    }
    spm_pop();

    u32 *dp = &asset_tex(0).px[0];
    u32 *sp = &lig->merged.px[0];
    for (i32 n = 0; n < 240 * 13; n++) {
        *dp++ &= *sp++;
    }
}

#if 0
void light_ray(game_s *g, l_light_s *lig, u32 ls, i32 w, v2_i32 p0, v2_i32 p1)
{
    i32 dx    = +abs_i32(p1.x - p0.x);
    i32 dy    = -abs_i32(p1.y - p0.y);
    i32 sx    = p0.x < p1.x ? +1 : -1;
    i32 sy    = p0.y < p1.y ? +1 : -1;
    i32 er    = dx + dy;
    i32 xa    = 1;
    i32 ya    = 1;
    u32 l     = 0;
    i32 x     = p0.x;
    i32 y     = p0.y;
    i32 steps = 0;
    i32 cx    = (lig->p.x - p0.x) >> 3;
    i32 cy    = (lig->p.y - p0.y) >> 3;

    while (l < ls) {
        u32 lv            = (u32)((ls - l) * 255) / ls;
        lig->l[x + y * w] = lv;

        if (steps) {
            l += 128;
            if ((--steps) == 0) break;
        } else {
            rec_i32 r  = {lig->p.x + ((x - p0.x) << 3),
                          lig->p.y + ((y - p0.y) << 3), 8, 8};
            bool32  px = map_traversable(g, r);
            if (!px) {
                steps = 4;
            }
        }

        i32 e2 = er << 1;
        if (e2 >= dy) { er += dy, x += sx, l += xa, xa += 2; }
        if (e2 <= dx) { er += dx, y += sy, l += ya, ya += 2; }
    }
}
#endif