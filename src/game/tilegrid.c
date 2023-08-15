// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================
/*
 * Tile collisions are based on pixels. Tiles are 16x16 and each tile shape
 * has a corresponding bitmask of solid(1)/empty(0) bits int[16].
 *
 * When checking if a solid pixel is overlapped we loop through the
 * tiles and determine the range in the corresponding bitmasks
 * to check against.
 */

#include "tilegrid.h"
#include "game.h"
#include "tab_collision.h"

tilegrid_s tilegrid_from_game(game_s *g)
{
        tilegrid_s tg =
            {g->tiles, g->tiles_x, g->tiles_y, g->pixel_x, g->pixel_y};
        return tg;
}

bool32 tiles_area(tilegrid_s tg, rec_i32 r)
{
        if (r.x < 0) return 1;
        if (r.y < 0) return 1;
        if (r.x + r.w > tg.pixel_x) return 1;
        if (r.y + r.h > tg.pixel_y) return 1;
        rec_i32 rgrid = {0, 0, tg.pixel_x, tg.pixel_y};
        rec_i32 riarea;
        intersect_rec(r, rgrid, &riarea);

        int px0 = riarea.x;
        int py0 = riarea.y;
        int px1 = riarea.x + riarea.w - 1;
        int py1 = riarea.y + riarea.h - 1;
        int tx0 = px0 >> 4; // divide by 16 (tile size)
        int ty0 = py0 >> 4;
        int tx1 = px1 >> 4;
        int ty1 = py1 >> 4;

        for (int ty = ty0; ty <= ty1; ty++) {
                int y0 = (ty == ty0 ? py0 & 15 : 0); // px in tile (local)
                int y1 = (ty == ty1 ? py1 & 15 : 15);
                for (int tx = tx0; tx <= tx1; tx++) {
                        int tl = tg.tiles[tx + ty * tg.tiles_x];
                        if (tl == 0) continue;
                        if (tl == 1) return 1;
                        int x0 = (tx == tx0 ? px0 & 15 : 0);
                        int x1 = (tx == tx1 ? px1 & 15 : 15);
                        int mk = (0xFFFF >> x0) & ~(0x7FFF >> x1);
                        // mk masks the collision data so we only see
                        // the relevant part    1---5
                        // px0 = 1, px1 = 5 -> 01111100

                        for (int py = y0; py <= y1; py++)
                                if (g_pxmask_tab[tl][py] & mk) return 1;
                }
        }
        return 0;
}

bool32 tiles_at(tilegrid_s tg, i32 x, i32 y)
{
        if (!(0 <= x && x < tg.pixel_x && 0 <= y && y < tg.pixel_y)) return 1;
        int tilex = x >> 4; /* "/ 16" */
        int tiley = y >> 4;
        int t     = tg.tiles[tilex + tiley * tg.tiles_x];
        if (t <= 1) return t; // ID = 0 all 0, ID = 1 all 1
        return (g_pxmask_tab[t][y & 15] & (0x8000 >> (x & 15)));
}

void tilegrid_bounds_minmax(game_s *g, v2_i32 pmin, v2_i32 pmax,
                            i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        *x1 = MAX(pmin.x, 0) >> 4; /* "/ 16" */
        *y1 = MAX(pmin.y, 0) >> 4;
        *x2 = MIN(pmax.x, g->pixel_x - 1) >> 4;
        *y2 = MIN(pmax.y, g->pixel_y - 1) >> 4;
}

void tilegrid_bounds_tri(game_s *g, tri_i32 t,
                         i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        v2_i32 pmin = v2_min(t.p[0], v2_min(t.p[1], t.p[2]));
        v2_i32 pmax = v2_max(t.p[0], v2_max(t.p[1], t.p[2]));
        tilegrid_bounds_minmax(g, pmin, pmax, x1, y1, x2, y2);
}

void tilegrid_bounds_rec(game_s *g, rec_i32 r,
                         i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        v2_i32 pmin = {r.x, r.y};
        v2_i32 pmax = {r.x + r.w, r.y + r.h};
        tilegrid_bounds_minmax(g, pmin, pmax, x1, y1, x2, y2);
}