// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
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

#define TC(X, Y) \
    {            \
        X << 3, Y << 3}

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

bool32 tile_map_hookable(g_s *g, rec_i32 r)
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
            // if (t == TILE_TYPE_THORNS || t == TILE_TYPE_SPIKES) continue;

            i32 x0 = (tx == tx0 ? px0 & 15 : 0);
            i32 x1 = (tx == tx1 ? px1 & 15 : 15);
            if (tile_solid_r(ID, x0, y0, x1, y1))
                return 1;
        }
    }
    return 0;
}

bool32 tile_map_solid(g_s *g, rec_i32 r)
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

bool32 tile_map_solid_pt(g_s *g, i32 x, i32 y)
{
    if (!(0 <= x && x < g->pixel_x && 0 <= y && y < g->pixel_y)) return 0;
    tile_s t  = g->tiles[(x >> 4) + (y >> 4) * g->tiles_x];
    i32    ID = t.collision;
    return (tile_solid_pt(ID, x & 15, y & 15));
}

bool32 tile_map_ladder_overlaps_rec(g_s *g, rec_i32 r, v2_i32 *tpos)
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
            switch (t) {
            default: break;
            case TILE_CLIMBWALL:
            case TILE_LADDER:
            case TILE_LADDER_ONE_WAY:
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

void tile_map_set_collision(g_s *g, rec_i32 r, i32 shape, i32 type)
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

bool32 map_blocked_by_solid(g_s *g, obj_s *o, rec_i32 r, i32 m)
{
    if (o->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS) {
        for (obj_each(g, it)) {
            if (it != o &&
                0 < it->mass &&
                m <= it->mass &&
                overlap_rec(r, obj_aabb(it))) {
                return 1;
            }
        }
    }
    return 0;
}

bool32 map_blocked_by_any_solid(g_s *g, rec_i32 r)
{
    for (obj_each(g, it)) {
        if (it->mass == 0) continue;
        if (overlap_rec(r, obj_aabb(it))) {
            return 1;
        }
    }
    return 0;
}

bool32 map_blocked_by_any_solid_pt(g_s *g, i32 x, i32 y)
{
    rec_i32 r = {x, y, 1, 1};
    return map_blocked_by_any_solid(g, r);
}

bool32 map_blocked(g_s *g, obj_s *o, rec_i32 r, i32 m)
{
    return ((o->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS) &&
            (tile_map_solid(g, r) || map_blocked_by_solid(g, o, r, m)));
}

bool32 map_blocked_pt(g_s *g, obj_s *o, i32 x, i32 y, i32 m)
{
    rec_i32 r = {x, y, 1, 1};
    return map_blocked(g, o, r, m);
}

bool32 map_traversable(g_s *g, rec_i32 r)
{
    if (tile_map_solid(g, r)) return 0;
    if (map_blocked_by_any_solid(g, r)) return 0;
    return 1;
}

bool32 map_traversable_pt(g_s *g, i32 x, i32 y)
{
    rec_i32 r = {x, y, 1, 1};
    return map_traversable(g, r);
}

i32 map_climbable_pt(g_s *g, i32 x, i32 y)
{
    if (0 <= x && x < g->pixel_x && 0 <= y && y < g->pixel_y) {
        tile_s t = g->tiles[(x >> 4) + (y >> 4) * g->tiles_x];
        if (tile_solid_pt(t.collision, x & 15, y & 15)) {
            switch (t.type) {
            case TILE_TYPE_DARK_OBSIDIAN:
                return MAP_CLIMBABLE_SLIPPERY;
            default:
                return MAP_CLIMBABLE_SUCCESS;
            }
        }
    }

    v2_i32 pt = {x, y};
    for (obj_each(g, it)) {
        if (it->mass == 0) continue;
        if (!(it->flags & OBJ_FLAG_CLIMBABLE)) continue;
        if (overlap_rec_pnt(obj_aabb(it), pt)) {
            return MAP_CLIMBABLE_SUCCESS;
        }
    }
    return MAP_CLIMBABLE_NO_TERRAIN;
}

tile_map_bounds_s tile_map_bounds_rec(g_s *g, rec_i32 r)
{
    v2_i32 pmin = {r.x, r.y};
    v2_i32 pmax = {r.x + r.w, r.y + r.h};
    return tile_map_bounds_pts(g, pmin, pmax);
}

tile_map_bounds_s tile_map_bounds_pts(g_s *g, v2_i32 p0, v2_i32 p1)
{
    tile_map_bounds_s b = {max_i32(p0.x - 1, 0) >> 4, // div 16
                           max_i32(p0.y - 1, 0) >> 4,
                           min_i32(p1.x + 1, g->pixel_x - 1) >> 4,
                           min_i32(p1.y + 1, g->pixel_y - 1) >> 4};
    return b;
}

tile_map_bounds_s tile_map_bounds_tri(g_s *g, tri_i32 t)
{
    v2_i32 pmin = v2_min3(t.p[0], t.p[1], t.p[2]);
    v2_i32 pmax = v2_max3(t.p[0], t.p[1], t.p[2]);
    return tile_map_bounds_pts(g, pmin, pmax);
}

tile_s *tile_map_at_pos(g_s *g, v2_i32 p)
{
    if (!(0 <= p.x && p.x < g->pixel_x)) return 0;
    if (!(0 <= p.y && p.y < g->pixel_y)) return 0;

    i32 tx = p.x / 16;
    i32 ty = p.y / 16;
    return &g->tiles[tx + ty * g->tiles_x];
}