// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "lighting.h"
#include "game.h"

void lighting_init(lighting_s *lig)
{
    lig->tex = tex_create_opaque(400, 240, asset_allocator);
}

#if 0 // shadowcasting
void lighting_apply_light(game_s *g, lighting_s *lig, l_light_s l, v2_i32 cam);

void lighting_do(game_s *g, lighting_s *lig, v2_i32 cam)
{
    tex_clr(lig->tex, GFX_COL_BLACK);

    for (int n = 0; n < lig->n_lights; n++) {
        lighting_apply_light(g, lig, lig->lights[n], cam);
    }

    tex_s tex = asset_tex(0);
    u32  *tx  = tex.px;
    u32  *sm  = lig->tex.px;
    i32   N   = tex.h * tex.wword;
    for (i32 n = 0; n < N; n++) {
        *tx++ &= *sm++;
    }
}

void lighting_apply_light(game_s *g, lighting_s *lig, l_light_s l, v2_i32 cam)
{
    spm_push();

    tex_s shadowmap_l = tex_create_opaque(lig->tex.w, lig->tex.h, spm_allocator);
    tex_clr(shadowmap_l, GFX_COL_BLACK);

    gfx_ctx_s ctx = gfx_ctx_default(shadowmap_l);

    u32 rsq = l.r * l.r;

    gfx_cir_fill(ctx, v2_add(l.p, cam), l.r << 1, PRIM_MODE_WHITE);

    rec_i32           rlig   = {l.p.x - l.r - 1, l.p.y - l.r - 1, l.r * 2 + 1, l.r * 2 + 1};
    tile_map_bounds_s bounds = tile_map_bounds_rec(g, rlig);

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            i32 coll = g->tiles[x + y * g->tiles_x].collision;
            if (!TILE_IS_SHAPE(coll)) continue;
            tile_corners_s corners = g_tile_corners[coll];
            v2_i32         ptile   = {x << 4, y << 4};

            for (i32 n = 0; n < corners.n; n++) {
                v2_i32 a  = v2_i32_from_v2_i8(corners.c[(n)]);
                v2_i32 b  = v2_i32_from_v2_i8(corners.c[(n + 1) % corners.n]);
                a         = v2_add(a, ptile);
                b         = v2_add(b, ptile);
                v2_i32 da = v2_sub(a, l.p);
                v2_i32 db = v2_sub(b, l.p);

                if (rsq <= v2_lensq(da) && rsq <= v2_lensq(db)) continue;

                da             = v2_shl(da, 8);
                db             = v2_shl(db, 8);
                a              = v2_add(a, cam);
                b              = v2_add(b, cam);
                v2_i32 poly[4] = {a, b, v2_add(b, db), v2_add(a, da)};
                gfx_poly_fill(ctx, poly, 4, PRIM_MODE_BLACK);
            }
        }
    }

    u32 *sp = lig->tex.px;
    u32 *sl = shadowmap_l.px;
    i32  N  = lig->tex.h * lig->tex.wword;
    for (i32 n = 0; n < N; n++) {
        *sp++ |= *sl++;
    }

    spm_pop();
}
#else // naive raycasting
void lighting_apply_light(game_s *g, lighting_s *lig, l_light_s l, v2_i32 cam);

void lighting_do(game_s *g, lighting_s *lig, v2_i32 cam)
{

    // memset(lig->l, 0, sizeof(lig->l));
    tex_clr(lig->tex, GFX_COL_BLACK);

    for (i32 n = 0; n < lig->n_lights; n++) {
        lighting_apply_light(g, lig, lig->lights[n], cam);
    }

    u32 *p1 = ((u32 *)asset_tex(0).px);
    u32 *p2 = ((u32 *)lig->tex.px);
    for (i32 y = 0; y < 240; y++) {
        for (i32 x = 0; x < 13; x++) {
            *p1++ &= *p2++;
        }
    }
}

void lighting_ray(game_s *g, lighting_s *lig, v2_i32 p1, v2_i32 p2, i32 ls, v2_i32 cam)
{
    i32       dx    = +abs_i32(p2.x - p1.x);
    i32       dy    = -abs_i32(p2.y - p1.y);
    i32       sx    = p1.x < p2.x ? +1 : -1;
    i32       sy    = p1.y < p2.y ? +1 : -1;
    i32       er    = dx + dy;
    i32       xa    = 1;
    i32       ya    = 1;
    i32       l     = 0;
    i32       x     = p1.x;
    i32       y     = p1.y;
    i32       steps = 0;
    gfx_ctx_s ctx   = gfx_ctx_default(lig->tex);

    while (1) {
        i32 tx = (x << 2) + cam.x;
        i32 ty = (y << 2) + cam.y;
        gfx_rec_fill(ctx, (rec_i32){tx, ty, 4, 4}, GFX_COL_WHITE);

        rec_i32 rr = {(x << 2), (y << 2), 4, 4};

        if (steps) {
            if (--steps == 0) break;
        } else {
            bool32 px = game_traversable(g, rr);
            if (!game_traversable(g, rr)) {
                steps = 4;
            }
        }

        if (ls <= l) break;
        i32 e2 = er << 1;
        if (e2 >= dy) { er += dy, x += sx, l += xa, xa += 2; }
        if (e2 <= dx) { er += dx, y += sy, l += ya, ya += 2; }
    }
}

void lighting_apply_light(game_s *g, lighting_s *lig, l_light_s l, v2_i32 cam)
{
    // cam.x &= ~3;
    // cam.y &= ~3;
    i32 x1 = (l.p.x - l.r - 1) >> 2;
    i32 y1 = (l.p.y - l.r - 1) >> 2;
    i32 x2 = (l.p.x + l.r + 1) >> 2;
    i32 y2 = (l.p.y + l.r + 1) >> 2;

    i32    ls = (l.r * l.r) / 16;
    v2_i32 pp = v2_shr(l.p, 2);

    for (i32 y = y1; y <= y2; y++) {
        v2_i32 pl = {x1, y};
        v2_i32 pr = {x2, y};
        lighting_ray(g, lig, pp, pl, ls, cam);
        lighting_ray(g, lig, pp, pr, ls, cam);
    }
    for (i32 x = x1; x <= x2; x++) {
        v2_i32 pu = {x, y1};
        v2_i32 pd = {x, y2};
        lighting_ray(g, lig, pp, pu, ls, cam);
        lighting_ray(g, lig, pp, pd, ls, cam);
    }
}
#endif