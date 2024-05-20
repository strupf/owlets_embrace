// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "tile_map.h"
#include "game.h"

bool32 tile_map_hookable(game_s *g, rec_i32 r)
{
    rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
    rec_i32 ri;
    if (!intersect_rec(r, rgrid, &ri)) return 0;

    i32 px0 = ri.x;
    i32 py0 = ri.y;
    i32 px1 = ri.x + ri.w - 1;
    i32 py1 = ri.y + ri.h - 1;
    i32 tx0 = px0 >> 4;
    i32 ty0 = py0 >> 4;
    i32 tx1 = px1 >> 4;
    i32 ty1 = py1 >> 4;

    for (i32 ty = ty0; ty <= ty1; ty++) {
        i32 y0 = (ty == ty0 ? py0 & 15 : 0); // px in tile (local)
        i32 y1 = (ty == ty1 ? py1 & 15 : 15);

        for (i32 tx = tx0; tx <= tx1; tx++) {
            tile_s tile = g->tiles[tx + ty * g->tiles_x];
            i32    c    = tile.collision;
            i32    t    = tile.type;
            if (!TILE_IS_SHAPE(c)) continue;
            if (!(t == TILE_TYPE_DIRT ||
                  t == TILE_TYPE_DIRT_DARK ||
                  t == TILE_TYPE_CLEAN))
                continue;
            if (c == TILE_BLOCK) return 1;
            i32 x0 = (tx == tx0 ? px0 & 15 : 0);
            i32 x1 = (tx == tx1 ? px1 & 15 : 15);
            i32 mk = (0xFFFF >> x0) & (0xFFFF << (15 - x1));
            for (i32 py = y0; py <= y1; py++)
                if (g_tile_px[c][py] & mk) return 1;
        }
    }
    return 0;
}

bool32 tile_map_solid(game_s *g, rec_i32 r)
{
    rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
    rec_i32 ri;
    if (!intersect_rec(r, rgrid, &ri)) return 0;

    i32 px0 = ri.x;
    i32 py0 = ri.y;
    i32 px1 = ri.x + ri.w - 1;
    i32 py1 = ri.y + ri.h - 1;
    i32 tx0 = px0 >> 4;
    i32 ty0 = py0 >> 4;
    i32 tx1 = px1 >> 4;
    i32 ty1 = py1 >> 4;

    for (i32 ty = ty0; ty <= ty1; ty++) {
        i32 y0 = (ty == ty0 ? py0 & 15 : 0); // px in tile (local)
        i32 y1 = (ty == ty1 ? py1 & 15 : 15);

        for (i32 tx = tx0; tx <= tx1; tx++) {
            i32 c = g->tiles[tx + ty * g->tiles_x].collision;
            if (!TILE_IS_SHAPE(c)) continue;
            if (TILE_IS_BLOCK(c)) return 1;
            i32 x0 = (tx == tx0 ? px0 & 15 : 0);
            i32 x1 = (tx == tx1 ? px1 & 15 : 15);
            u32 mk = (0xFFFF >> x0) & (0xFFFF << (15 - x1));
            for (i32 py = y0; py <= y1; py++)
                if (g_tile_px[c][py] & mk) return 1;
        }
    }
    return 0;
}
bool32 tile_map_solid_pt(game_s *g, i32 x, i32 y)
{
    if (!(0 <= x && x < g->pixel_x && 0 <= y && y < g->pixel_y)) return 0;

    i32 ID = g->tiles[(x >> 4) + (y >> 4) * g->tiles_x].collision;
    if (!TILE_IS_SHAPE(ID)) return 0;
    return (g_tile_px[ID][y & 15] & (0x8000 >> (x & 15)));
}

bool32 tile_map_one_way(game_s *g, rec_i32 r)
{
    rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
    rec_i32 ri;
    if (!intersect_rec(r, rgrid, &ri)) return 0;

    i32 tx0 = (ri.x) >> 4;
    i32 ty0 = (ri.y) >> 4;
    i32 tx1 = (ri.x + ri.w - 1) >> 4;
    i32 ty1 = (ri.y + ri.h - 1) >> 4;

    for (i32 ty = ty0; ty <= ty1; ty++) {
        for (i32 tx = tx0; tx <= tx1; tx++) {
            i32 t = g->tiles[tx + ty * g->tiles_x].collision;
            if (t == TILE_ONE_WAY || t == TILE_LADDER_ONE_WAY)
                return 1;
        }
    }
    return 0;
}

bool32 tile_map_ladder_overlaps_rec(game_s *g, rec_i32 r, v2_i32 *tpos)
{
    rec_i32 ri;
    rec_i32 rroom = {0, 0, g->pixel_x, g->pixel_y};
    if (!intersect_rec(rroom, r, &ri)) return 0;

    i32 tx1 = (ri.x) >> 4;
    i32 ty1 = (ri.y) >> 4;
    i32 tx2 = (ri.x + ri.w - 1) >> 4;
    i32 ty2 = (ri.y + ri.h - 1) >> 4;
    for (i32 ty = ty1; ty <= ty2; ty++) {
        for (i32 tx = tx1; tx <= tx2; tx++) {
            i32 t = g->tiles[tx + ty * g->tiles_x].collision;
            if (t == TILE_LADDER || t == TILE_LADDER_ONE_WAY) {
                if (tpos) {
                    tpos->x = tx;
                    tpos->y = ty;
                }
                return 1;
            }
        }
    }
    return 0;
}

void tile_map_set_collision(game_s *g, rec_i32 r, i32 shape, i32 type)
{
    i32 tx = r.x >> 4;
    i32 ty = r.y >> 4;
    i32 nx = r.w >> 4;
    i32 ny = r.h >> 4;

    for (i32 y = 0; y < ny; y++) {
        for (i32 x = 0; x < nx; x++) {
            tile_s *t    = &g->tiles[x + tx + (y + ty) * g->tiles_x];
            t->collision = shape;
            t->type      = type;
        }
    }

    if (TILE_IS_SHAPE(shape)) {
        game_on_solid_appear(g);
    }
}

tile_map_bounds_s tile_map_bounds_rec(game_s *g, rec_i32 r)
{
    v2_i32 pmin = {r.x, r.y};
    v2_i32 pmax = {r.x + r.w, r.y + r.h};
    return tile_map_bounds_pts(g, pmin, pmax);
}

tile_map_bounds_s tile_map_bounds_pts(game_s *g, v2_i32 p0, v2_i32 p1)
{
    tile_map_bounds_s b = {max_i32(p0.x - 1, 0) >> 4, // div 16
                           max_i32(p0.y - 1, 0) >> 4,
                           min_i32(p1.x + 1, g->pixel_x - 1) >> 4,
                           min_i32(p1.y + 1, g->pixel_y - 1) >> 4};
    return b;
}

tile_map_bounds_s tile_map_bounds_tri(game_s *g, tri_i32 t)
{
    v2_i32 pmin = v2_min3(t.p[0], t.p[1], t.p[2]);
    v2_i32 pmax = v2_max3(t.p[0], t.p[1], t.p[2]);
    return tile_map_bounds_pts(g, pmin, pmax);
}