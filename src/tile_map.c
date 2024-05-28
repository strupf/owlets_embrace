// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "tile_map.h"
#include "game.h"

bool32 tile_solid_pt(i32 ID, i32 x, i32 y)
{
    switch (ID) {
    case TILE_EMPTY: return 0;
    case TILE_BLOCK: return 1;
    case TILE_SLOPE_45_0:
        if ((15 - x) <= y) return 1;
        break;
    case TILE_SLOPE_45_3:
        if ((15 - x) >= y) return 1;
        break;
    case TILE_SLOPE_45_1:
        if (x >= y) return 1;
        break;
    case TILE_SLOPE_45_2:
        if (x <= y) return 1;
        break;
    }
    return 0;
}

bool32 tile_solid_r(i32 ID, i32 x0, i32 y0, i32 x1, i32 y1)
{
    switch (ID) {
    case TILE_EMPTY: return 0;
    case TILE_BLOCK: return 1;
    case TILE_SLOPE_45_0:
        if ((15 - x1) <= y1) return 1;
        break;
    case TILE_SLOPE_45_3:
        if ((15 - x0) >= y0) return 1;
        break;
    case TILE_SLOPE_45_1:
        if (x1 >= y0) return 1;
        break;
    case TILE_SLOPE_45_2:
        if (x0 <= y1) return 1;
        break;
    }
    return 0;
}

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
            i32    ID   = tile.collision;
            i32    t    = tile.type;
            if (!TILE_IS_SHAPE(ID)) continue;
            if (!(t == TILE_TYPE_DIRT))
                continue;
            i32 x0 = (tx == tx0 ? px0 & 15 : 0);
            i32 x1 = (tx == tx1 ? px1 & 15 : 15);
            if (tile_solid_r(ID, x0, y0, x1, y1))
                return 1;
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
            i32 ID = g->tiles[tx + ty * g->tiles_x].collision;
            i32 x0 = (tx == tx0 ? px0 & 15 : 0);
            i32 x1 = (tx == tx1 ? px1 & 15 : 15);
            if (tile_solid_r(ID, x0, y0, x1, y1))
                return 1;
        }
    }
    return 0;
}

bool32 tile_map_solid_pt(game_s *g, i32 x, i32 y)
{
    if (!(0 <= x && x < g->pixel_x && 0 <= y && y < g->pixel_y)) return 0;
    i32 ID = g->tiles[(x >> 4) + (y >> 4) * g->tiles_x].collision;
    return (tile_solid_pt(ID, x & 15, y & 15));
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

bool32 map_overlaps_mass_eq_or_higher(game_s *g, rec_i32 r, i32 m)
{
    return map_blocked(g, NULL, r, m);
}

bool32 map_blocked_by_solid(game_s *g, obj_s *o, rec_i32 r, i32 m)
{
    for (obj_each(g, it)) {
        if (it == o) continue;
        if (it->mass == 0) continue;
        if (m <= it->mass && overlap_rec(r, obj_aabb(it))) {
            obj_s *ocarry = (o && o->ID == OBJ_ID_HERO) ? carryable_present(g) : NULL;
            if (ocarry == it) continue;
            return 1;
        }
    }
    return 0;
}

bool32 map_blocked_by_any_solid(game_s *g, rec_i32 r)
{
    for (obj_each(g, it)) {
        if (it->mass == 0) continue;
        if (overlap_rec(r, obj_aabb(it))) {
            return 1;
        }
    }
    return 0;
}

bool32 map_blocked_by_any_solid_pt(game_s *g, i32 x, i32 y)
{
    v2_i32 pt = {x, y};
    for (obj_each(g, it)) {
        if (it->mass == 0) continue;
        if (overlap_rec_pnt(obj_aabb(it), pt)) {
            return 1;
        }
    }
    return 0;
}

bool32 map_blocked(game_s *g, obj_s *o, rec_i32 r, i32 m)
{
    if (tile_map_solid(g, r)) return 1;
    return (map_blocked_by_solid(g, o, r, m));
}

bool32 map_blocked_pt(game_s *g, obj_s *o, i32 x, i32 y, i32 m)
{
    if (tile_map_solid_pt(g, x, y)) return 0;
    rec_i32 r = {x, y, 1, 1};
    return (map_blocked_by_solid(g, NULL, r, 0));
}

bool32 map_traversable(game_s *g, rec_i32 r)
{
    if (tile_map_solid(g, r)) return 0;
    if (map_blocked_by_any_solid(g, r)) return 0;
    return 1;
}

bool32 map_traversable_pt(game_s *g, i32 x, i32 y)
{
    if (tile_map_solid_pt(g, x, y)) return 0;
    if (map_blocked_by_any_solid_pt(g, x, y)) return 0;
    return 1;
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