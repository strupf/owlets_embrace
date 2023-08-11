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

#include "collision.h"
#include "tab_collision.h"

bool32 tiles_area(tilegrid_s tg, rec_i32 r)
{
        rec_i32 rgrid = {0, 0, tg.pixel_x, tg.pixel_y};
        rec_i32 riarea;
        if (!intersect_rec(r, rgrid, &riarea)) return 0;

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

bool32 tiles_area_row(tilegrid_s tg, rec_i32 r)
{
        ASSERT(r.h == 1);
        rec_i32 rgrid = {0, 0, tg.pixel_x, tg.pixel_y};
        rec_i32 riarea;
        if (!intersect_rec(r, rgrid, &riarea)) return 0;

        int px0 = riarea.x;
        int py0 = riarea.y;
        int px1 = riarea.x + riarea.w - 1;
        int tx0 = px0 >> 4; // divide by 16 (tile size)
        int tx1 = px1 >> 4;
        int ty  = py0 >> 4;
        int py  = (py0 & 15); // px in tile (local)
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

                if (g_pxmask_tab[tl][py] & mk) return 1;
        }
        return 0;
}

bool32 tiles_at(tilegrid_s tg, i32 x, i32 y)
{
        if (!(0 <= x && x < tg.pixel_x && 0 <= y && y < tg.pixel_y)) return 0;
        int tilex = x >> 4; /* "/ 16" */
        int tiley = y >> 4;
        int t     = tg.tiles[tilex + tiley * tg.tiles_x];
        if (t <= 1) return t; // ID = 0 all 0, ID = 1 all 1
        return (g_pxmask_tab[t][y & 15] & (0x8000 >> (x & 15)));
}