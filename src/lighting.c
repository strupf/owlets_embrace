// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "lighting.h"
#include "game.h"

void lighting_init(lighting_s *lig)
{
}

// naive raycasting
void lighting_apply_light(game_s *g, lighting_s *lig, l_light_s l, v2_i32 cam);

void lighting_do(game_s *g, lighting_s *lig, v2_i32 cam)
{
    memset(lig->l, 0, sizeof(lig->l));
    memset(lig->l3, 0, sizeof(lig->l3));

    for (i32 n = 0; n < lig->n_lights; n++) {
        lighting_apply_light(g, lig, lig->lights[n], cam);
    }

    for (i32 y = 0; y < LIGHTING_H2; y++) {
        for (i32 x = 0; x < LIGHTING_W2; x++) {
            i32 ll = lig->l3[x + y * LIGHTING_W2];
            i32 u1 = ((x << 3) + (cam.x & 7));
            i32 v1 = ((y << 3) + (cam.y & 7));
            for (i32 yy = 0; yy < 8; yy++) {
                for (i32 xx = 0; xx < 8; xx++) {
                    lig->l[(u1 + xx) + (v1 + yy) * LIGHTING_W] = ll;
                }
            }
        }
    }

#if 0 // smoothing
    for (i32 y = 2; y < LIGHTING_H - 2; y++) {
        for (i32 x = 2; x < LIGHTING_W - 2; x++) {
            u32 sum = (5 * 5) - 1;
            for (i32 yi = -2; yi <= +2; yi++) {
                for (i32 xi = -2; xi <= +2; xi++) {
                    i32 xx = x + xi;
                    i32 yy = y + yi;
                    sum += lig->l2[xx + yy * LIGHTING_W];
                }
            }
            lig->l[x + y * LIGHTING_W] = sum / (5 * 5);
        }
    }
#endif

    // dither
    u32 *p1 = ((u32 *)asset_tex(0).px);
    for (i32 y = 0; y < 240; y++) {
        i32 yl = (y + 8);
        i32 j  = yl & 3;
        for (i32 x = 0; x < 13; x++) {
            u32 p = 0;
            for (i32 b = 0; b < 32; b++) {
                i32 xl  = (x << 5) + b + 8;
                i32 col = lig->l[xl + yl * LIGHTING_W];
                i32 i   = xl & 3;

                static const i32 MM[4][4] = {{0, 8, 2, 10},
                                             {12, 4, 14, 6},
                                             {3, 11, 1, 9},
                                             {15, 7, 13, 5}};
                if (col > MM[i][j]) {
                    p |= 0x80000000U >> b;
                }
            }
            *p1++ &= bswap32(p);
        }
    }
}

void lighting_ray(game_s *g, lighting_s *lig, v2_i32 p1, v2_i32 p2, u32 ls, v2_i32 cam)
{
    i32 dx    = +abs_i32(p2.x - p1.x);
    i32 dy    = -abs_i32(p2.y - p1.y);
    i32 sx    = p1.x < p2.x ? +1 : -1;
    i32 sy    = p1.y < p2.y ? +1 : -1;
    i32 er    = dx + dy;
    i32 xa    = 1;
    i32 ya    = 1;
    u32 l     = 0;
    i32 x     = p1.x;
    i32 y     = p1.y;
    i32 steps = 0;

    while (l < ls) {
        i32 tx = x + cam.x; // display coordinates
        i32 ty = y + cam.y;

        if (0 <= tx && tx < LIGHTING_W2 && 0 <= ty && ty < LIGHTING_H2) {
            lig->l3[tx + ty * LIGHTING_W2] = (u32)((ls - l) << 4) / ls;
        }

        if (steps) {
            if (--steps == 0) break;
        } else {
            rec_i32 rr = {(x << 3) - 4, (y << 3) - 4, 8, 8};
            bool32  px = game_traversable(g, rr);
            if (!px) {
                steps = 3;
            }
        }

        i32 e2 = er << 1;
        if (e2 >= dy) { er += dy, x += sx, l += xa, xa += 2; }
        if (e2 <= dx) { er += dx, y += sy, l += ya, ya += 2; }
    }
}

void lighting_apply_light(game_s *g, lighting_s *lig, l_light_s l, v2_i32 cam)
{
    i32 rr = l.r;
    i32 x1 = ((l.p.x - rr - 1) >> 3);
    i32 y1 = ((l.p.y - rr - 1) >> 3);
    i32 x2 = ((l.p.x + rr + 1) >> 3);
    i32 y2 = ((l.p.y + rr + 1) >> 3);

    i32    ls = (rr * rr) >> (3 * 2);
    v2_i32 pp = {(l.p.x + 4) >> 3, (l.p.y + 4) >> 3};
    v2_i32 cm = v2_shr(cam, 3);

    for (i32 y = y1; y <= y2; y++) {
        v2_i32 pl = {x1, y};
        v2_i32 pr = {x2, y};
        lighting_ray(g, lig, pp, pl, ls, cm);
        lighting_ray(g, lig, pp, pr, ls, cm);
    }
    for (i32 x = x1; x <= x2; x++) {
        v2_i32 pu = {x, y1};
        v2_i32 pd = {x, y2};
        lighting_ray(g, lig, pp, pu, ls, cm);
        lighting_ray(g, lig, pp, pd, ls, cm);
    }
}