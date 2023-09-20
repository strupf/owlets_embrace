// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "tab_collision.h"

static bool32 room_tiles_area(game_s *g, rec_i32 r);

bool32 room_area_blocked(game_s *g, rec_i32 r)
{
        if (room_tiles_area(g, r)) return 1;
        obj_listc_s solids = objbucket_list(g, OBJ_BUCKET_SOLID);
        for (int n = 0; n < solids.n; n++) {
                obj_s *o = solids.o[n];
                if (overlap_rec_excl(obj_aabb(o), r))
                        return 1;
        }
        return 0;
}

bool32 room_is_ladder(game_s *g, v2_i32 p)
{
        if (!(0 <= p.x && p.x < g->pixel_x && 0 <= p.y && p.y < g->pixel_y))
                return 0;
        return g->tiles[(p.x >> 4) + (p.y >> 4) * g->tiles_x] == TILE_LADDER;
}

static bool32 room_tiles_area(game_s *g, rec_i32 r)
{
        if (r.x < 0) return 1;
        if (r.y < 0) return 1;
        if (r.x + r.w > g->pixel_x) return 1;
        if (r.y + r.h > g->pixel_y) return 1;
        rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
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
                        int tl = g->tiles[tx + ty * g->tiles_x];
                        if (!(0 < tl && tl < NUM_TILE_BLOCKS)) continue;
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

bool32 tiles_at(game_s *g, i32 x, i32 y)
{
        if (!(0 <= x && x < g->pixel_x && 0 <= y && y < g->pixel_y)) return 1;
        int tilex = x >> 4; /* "/ 16" */
        int tiley = y >> 4;
        int t     = g->tiles[tilex + tiley * g->tiles_x];
        if (t <= 1) return t; // ID = 0 all 0, ID = 1 all 1
        if (!(0 < t && t < NUM_TILE_BLOCKS)) return 0;
        return (g_pxmask_tab[t][y & 15] & (0x8000 >> (x & 15)));
}

void room_tilebounds_pts(game_s *g, v2_i32 p1, v2_i32 p2, i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        *x1 = max_i(p1.x, 0) >> 4; /* "/ 16" */
        *y1 = max_i(p1.y, 0) >> 4;
        *x2 = min_i(p2.x, g->pixel_x - 1) >> 4;
        *y2 = min_i(p2.y, g->pixel_y - 1) >> 4;
}

void room_tilebounds_tri(game_s *g, tri_i32 t, i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        v2_i32 pmin = v2_min(t.p[0], v2_min(t.p[1], t.p[2]));
        v2_i32 pmax = v2_max(t.p[0], v2_max(t.p[1], t.p[2]));
        room_tilebounds_pts(g, pmin, pmax, x1, y1, x2, y2);
}

void room_tilebounds_rec(game_s *g, rec_i32 r, i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        v2_i32 pmin = {r.x, r.y};
        v2_i32 pmax = {r.x + r.w, r.y + r.h};
        room_tilebounds_pts(g, pmin, pmax, x1, y1, x2, y2);
}