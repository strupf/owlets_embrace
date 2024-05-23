// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "tile.h"

tile_walls_s tile_walls_get(i32 ID)
{
    tile_walls_s   res = {0};
    tile_corners_s tc  = g_tile_corners[ID];
    res.n              = tc.n;
    for (i32 i = 0; i < tc.n; i++) {
        v2_i32      a = v2_i32_from_v2_i8(tc.c[i]);
        v2_i32      b = v2_i32_from_v2_i8(tc.c[(i + 1) % tc.n]);
        v2_i32      d = {b.y - a.y, a.x - b.x};
        i32         l = v2_len(d);
        v2_i32      n = {((d.x << 8) + 1) / l, ((d.y << 8) + 1) / l};
        tile_wall_s w = {a, b, n};
        res.w[i]      = w;
    }
    return res;
}

tile_tris_s tile_tris_get(i32 ID)
{
    tile_corners_s tc   = g_tile_corners[ID];
    v2_i32         p[4] = {v2_i32_from_v2_i8(tc.c[0]),
                           v2_i32_from_v2_i8(tc.c[1]),
                           v2_i32_from_v2_i8(tc.c[2]),
                           v2_i32_from_v2_i8(tc.c[3])};
    tri_i32        t1   = {{p[0], p[1], p[2]}};
    tri_i32        t2   = {{p[0], p[2], p[3]}};
    tile_tris_s    res  = {tc.n - 2, {t1, t2}};
    return res;
}

const u16 g_tile_px[NUM_TILE_SHAPES][16] = {
    {0x0000U, 0x0000U, 0x0000U, 0x0000U, 0x0000U, 0x0000U, 0x0000U, 0x0000U, // empty
     0x0000U, 0x0000U, 0x0000U, 0x0000U, 0x0000U, 0x0000U, 0x0000U, 0x0000U},
    //
    {0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, // block
     0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU},
    //
    {0x0001U, 0x0003U, 0x0007U, 0x000FU, 0x001FU, 0x003FU, 0x007FU, 0x00FFU, // SLOPES 45
     0x01FFU, 0x03FFU, 0x07FFU, 0x0FFFU, 0x1FFFU, 0x3FFFU, 0x7FFFU, 0xFFFFU},
    //
    {0xFFFFU, 0x7FFFU, 0x3FFFU, 0x1FFFU, 0x0FFFU, 0x07FFU, 0x03FFU, 0x01FFU,
     0x00FFU, 0x007FU, 0x003FU, 0x001FU, 0x000FU, 0x0007U, 0x0003U, 0x0001U},
    //
    {0x8000U, 0xC000U, 0xE000U, 0xF000U, 0xF800U, 0xFC00U, 0xFE00U, 0xFF00U,
     0xFF80U, 0xFFC0U, 0xFFE0U, 0xFFF0U, 0xFFF8U, 0xFFFCU, 0xFFFEU, 0xFFFFU},
    //
    {0xFFFFU, 0xFFFEU, 0xFFFCU, 0xFFF8U, 0xFFF0U, 0xFFE0U, 0xFFC0U, 0xFF80U,
     0xFF00U, 0xFE00U, 0xFC00U, 0xF800U, 0xF000U, 0xE000U, 0xC000U, 0x8000U}
    //
};

#define TC(X, Y)       \
    {                  \
        X << 3, Y << 3 \
    }

const tile_corners_s g_tile_corners[NUM_TILE_SHAPES] = {
    {0, {TC(0, 0), TC(0, 0), TC(0, 0), TC(0, 0)}}, // empty
    {4, {TC(0, 0), TC(2, 0), TC(2, 2), TC(0, 2)}}, // block
    //
    {3, {TC(0, 2), TC(2, 0), TC(2, 2)}}, // slope 45
    {3, {TC(0, 0), TC(2, 0), TC(2, 2)}},
    {3, {TC(0, 0), TC(2, 2), TC(0, 2)}},
    {3, {TC(0, 0), TC(2, 0), TC(0, 2)}}};

// triangle coordinates
// x0 y0 x1 y1 x2 y2
const i32 g_tile_tris[NUM_TILE_SHAPES * 12] = {
    // dummy triangles
    0, 0, 0, 0, 0, 0, // empty
    0, 0, 0, 0, 0, 0, // solid
    // slope 45
    0, 16, 16, 16, 16, 0, // 2
    0, 0, 16, 0, 16, 16,  // 3
    0, 0, 0, 16, 16, 16,  // 4
    0, 0, 0, 16, 16, 0    // 5
    //
};