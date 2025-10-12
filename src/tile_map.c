// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "tile_map.h"
#include "game.h"

bool32 tile_solid_pt(i32 shape, i32 x, i32 y)
{
    switch (shape) {
    case TILE_BLOCK: return 1;
    case TILE_SLOPE_45_0: return ((15 - x) <= y);
    case TILE_SLOPE_45_3: return ((15 - x) >= y);
    case TILE_SLOPE_45_1: return (x >= y);
    case TILE_SLOPE_45_2: return (x <= y);
    }
    return 0;
}

bool32 tile_solid_r(i32 shape, i32 x0, i32 y0, i32 x1, i32 y1)
{
    switch (shape) {
    case TILE_BLOCK: return 1;
    case TILE_SLOPE_45_0: return ((15 - x1) <= y1);
    case TILE_SLOPE_45_3: return ((15 - x0) >= y0);
    case TILE_SLOPE_45_1: return (x1 >= y0);
    case TILE_SLOPE_45_2: return (x0 <= y1);
    }
    return 0;
}

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

const tri_i16 g_tiletris[NUM_TILE_SHAPES] = {
    // dummy triangles
    {{{0, 0}, {0, 0}, {0, 0}}},     // empty
    {{{0, 0}, {0, 0}, {0, 0}}},     // solid
                                    // slope 45
    {{{0, 16}, {16, 16}, {16, 0}}}, // 2
    {{{0, 0}, {16, 0}, {16, 16}}},  // 3
    {{{0, 0}, {0, 16}, {16, 16}}},  // 4
    {{{0, 0}, {0, 16}, {16, 0}}}    // 5
    //
};

tile_s tile_map_get_at(g_s *g, i32 tx, i32 ty)
{
    i32 x = clamp_i32(tx, 0, g->tiles_x - 1);
    i32 y = clamp_i32(ty, 0, g->tiles_y - 1);
    return g->tiles[x + y * g->tiles_x];
}

bool32 tile_map_hookable(g_s *g, rec_i32 r)
{
    // outer edge tiles are projected into infinity for
    // tile positions out of the level
    i32 px0 = r.x;
    i32 py0 = r.y;
    i32 px1 = r.x + r.w - 1;
    i32 py1 = r.y + r.h - 1;
    i32 tx0 = px0 >> 4;
    i32 ty0 = py0 >> 4;
    i32 tx1 = px1 >> 4;
    i32 ty1 = py1 >> 4;

    for (i32 ty = ty0; ty <= ty1; ty++) {
        i32 y0 = (ty == ty0 ? py0 & 15 : 0); // px in tile (local)
        i32 y1 = (ty == ty1 ? py1 & 15 : 15);
        i32 tv = clamp_i32(ty, 0, g->tiles_y - 1);

        for (i32 tx = tx0; tx <= tx1; tx++) {
            i32    tu   = clamp_i32(tx, 0, g->tiles_x - 1);
            tile_s tile = g->tiles[tu + tv * g->tiles_x];
            i32    ID   = tile.shape;
            i32    t    = tile.type;
            if (!TILE_IS_SHAPE(ID)) continue;
            // if (t == TILE_TYPE_THORNS || t == TILE_TYPE_SPIKES) continue;

            i32 x0 = (tx == tx0 ? px0 & 15 : 0);
            i32 x1 = (tx == tx1 ? px1 & 15 : 15);
            if (tile_solid_r(ID, x0, y0, x1, y1)) {
                return 1;
            }
        }
    }
    return 0;
}

bool32 tile_map_solid(g_s *g, rec_i32 r)
{
    // outer edge tiles are projected into infinity for
    // tile positions out of the level
    i32 px0 = r.x;
    i32 py0 = r.y;
    i32 px1 = r.x + r.w - 1;
    i32 py1 = r.y + r.h - 1;
    i32 tx0 = px0 >> 4;
    i32 ty0 = py0 >> 4;
    i32 tx1 = px1 >> 4;
    i32 ty1 = py1 >> 4;

    for (i32 ty = ty0; ty <= ty1; ty++) {
        i32 y0 = (ty == ty0 ? py0 & 15 : 0); // px in tile (local)
        i32 y1 = (ty == ty1 ? py1 & 15 : 15);
        i32 tv = clamp_i32(ty, 0, g->tiles_y - 1);

        for (i32 tx = tx0; tx <= tx1; tx++) {
            i32    tu   = clamp_i32(tx, 0, g->tiles_x - 1);
            tile_s tile = g->tiles[tu + tv * g->tiles_x];
            i32    ID   = tile.shape;
            i32    x0   = (tx == tx0 ? px0 & 15 : 0);
            i32    x1   = (tx == tx1 ? px1 & 15 : 15);
            if (tile_solid_r(ID, x0, y0, x1, y1)) {
                return 1;
            }
        }
    }
    return 0;
}

bool32 tile_map_solid_pt(g_s *g, i32 x, i32 y)
{
    // outer edge tiles are projected into infinity for
    // tile positions out of the level
    i32    tx = clamp_i32(x >> 4, 0, g->tiles_x - 1);
    i32    ty = clamp_i32(y >> 4, 0, g->tiles_y - 1);
    tile_s t  = g->tiles[tx + ty * g->tiles_x];
    return (tile_solid_pt(t.shape, x & 15, y & 15));
}

void tile_map_set_collision(g_s *g, rec_i32 r, i32 shape, i32 type)
{
    i32 tx = r.x >> 4;
    i32 ty = r.y >> 4;
    i32 nx = r.w >> 4;
    i32 ny = r.h >> 4;

    for (i32 y = 0; y < ny; y++) {
        for (i32 x = 0; x < nx; x++) {
            i32 u = x + tx;
            i32 v = y + ty;
            if ((u32)u < (u32)g->tiles_x && (u32)v < (u32)g->tiles_y) {
                tile_s *t = &g->tiles[u + v * g->tiles_x];
                t->shape  = shape;
                t->type   = type;
            }
        }
    }

    if (TILE_IS_SHAPE(shape)) {
        game_on_solid_appear(g);
    }
}

bool32 map_blocked_excl_offs(g_s *g, rec_i32 r, obj_s *o, i32 dx, i32 dy)
{
    if (o && !(o->moverflags & OBJ_MOVER_TERRAIN_COLLISIONS)) return 0;

    rec_i32 ri = {r.x + dx, r.y + dy, r.w, r.h};
    if (tile_map_solid(g, ri)) return 1;

    for (obj_each(g, i)) {
        if (i != o && (i->flags & OBJ_FLAG_SOLID) && overlap_rec(r, obj_aabb(i)) &&
            !obj_ignores_solid(o, i, 0)) {
            return 1;
        }
    }
    return 0;
}

bool32 map_blocked_offs(g_s *g, rec_i32 r, i32 dx, i32 dy)
{
    return map_blocked_excl_offs(g, r, 0, dx, dy);
}

bool32 map_blocked_excl(g_s *g, rec_i32 r, obj_s *o)
{
    return map_blocked_excl_offs(g, r, o, 0, 0);
}

bool32 map_blocked(g_s *g, rec_i32 r)
{
    return map_blocked_excl_offs(g, r, 0, 0, 0);
}

bool32 map_blocked_pt(g_s *g, i32 x, i32 y)
{
    rec_i32 r = {x, y, 1, 1};
    return map_blocked_excl_offs(g, r, 0, 0, 0);
}

i32 map_climbable_pt(g_s *g, i32 x, i32 y)
{
    i32    tx = clamp_i32(x >> 4, 0, g->tiles_x - 1);
    i32    ty = clamp_i32(y >> 4, 0, g->tiles_y - 1);
    tile_s t  = g->tiles[tx + ty * g->tiles_x];

    if (tile_solid_pt(t.shape, x & 15, y & 15)) {
        switch (t.type) {
        case TILE_TYPE_DARK_OBSIDIAN:
            return MAP_CLIMBABLE_SLIPPERY;
        default:
            return MAP_CLIMBABLE_SUCCESS;
        }
    }

    v2_i32 pt = {x, y};
    for (obj_each(g, it)) {
        if (!(it->flags & OBJ_FLAG_SOLID)) continue;
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
    i32 tx = clamp_i32(p.x >> 4, 0, g->tiles_x - 1);
    i32 ty = clamp_i32(p.y >> 4, 0, g->tiles_y - 1);
    return &g->tiles[tx + ty * g->tiles_x];
}

// if tile type a connects to a neighbour tiletype b
static bool32 at_types_blending(i32 a, i32 b)
{
    if (a == b) return 1;
    if (b == TILE_TYPE_INVISIBLE_NON_CONNECTING ||
        b == TILE_TYPE_DARK_OBSIDIAN) return 0;
    if (b == TILE_TYPE_INVISIBLE_CONNECTING ||
        a == TILE_TYPE_THORNS ||
        a == TILE_TYPE_SPIKES ||
        a == TILE_TYPE_DARK_OBSIDIAN) return 1;
    if (b == TILE_TYPE_THORNS ||
        b == TILE_TYPE_SPIKES) return 0;

    if (a == 25) return 0;
    if (tile_type_color(a) == tile_type_color(b)) return 1;
    if (tile_type_render_priority(a) < tile_type_render_priority(b)) return 1;
    return 0;
}

static void autotile_do_at(tile_s *tiles, i32 w, i32 h, i32 x, i32 y, u32 *seed_visuals);

static bool32 autotile_is_inner_gradient(tile_s *tiles, i32 w, i32 h, i32 x, i32 y)
{
    tile_s *t = &tiles[x + y * w];
    if (tile_get_shape(t) != TILE_BLOCK) return 0;

    i32 titype = tile_get_type(t);
    switch (titype) {
    default: return 0;
    case TILE_TYPE_BRIGHT_SNOW:
    case TILE_TYPE_BRIGHT_MOUNTAIN:
    case TILE_TYPE_BRIGHT_STONE: break;
    }

    for (i32 yy = -1; yy <= +1; yy++) {
        for (i32 xx = -1; xx <= +1; xx++) {
            if (xx == 0 && yy == 0) continue;

            i32 u = x + xx;
            i32 v = y + yy;

            // cast to unsigned
            // if negative: wraps around and is much bigger than w
            // therefore checked for out of bounds
            if ((u32)u < (u32)w && (u32)v < (u32)h) {
                tile_s *tj = &tiles[u + v * w];

                if (tile_get_type(tj) != titype || tile_get_shape(tj) != TILE_BLOCK) {
                    return 0;
                }
            }
        }
    }

    return 1;
}

void autotile_terrain_section(tile_s *tiles, i32 w, i32 h, i32 offx, i32 offy,
                              i32 rx, i32 ry, i32 rw, i32 rh)
{
    i32 y1 = max_i32(ry, 0);
    i32 x1 = max_i32(rx, 0);
    i32 y2 = min_i32(ry + rh, h) - 1;
    i32 x2 = min_i32(rx + rw, w) - 1;

    for (i32 y = y1; y <= y2; y++) {
        for (i32 x = x1; x <= x2; x++) {
            tile_s *t = &tiles[x + y * w];
            if (autotile_is_inner_gradient(tiles, w, h, x, y)) {
                t->type |= TILE_TYPE_FLAG_INNER_GRADIENT;
            } else {
                t->type &= ~TILE_TYPE_FLAG_INNER_GRADIENT;
            }
        }
    }
    for (i32 y = y1; y <= y2; y++) {
        for (i32 x = x1; x <= x2; x++) {
            u32 s = (u32)2903735 + (u32)((x + offx) ^ (y + offy)) * 1234807;
            autotile_do_at(tiles, w, h, x, y, &s);
        }
    }
}

void autotile_terrain_section_xy(tile_s *tiles, i32 w, i32 h, i32 tx1, i32 ty1, i32 tx2, i32 ty2, i32 offx, i32 offy)
{
    i32 x1 = max_i32(tx1 - 2, 0);
    i32 y1 = max_i32(ty1 - 2, 0);
    i32 x2 = min_i32(tx2 + 2, w - 1);
    i32 y2 = min_i32(ty2 + 2, h - 1);

    for (i32 y = y1; y <= y2; y++) {
        for (i32 x = x1; x <= x2; x++) {
            tile_s *t = &tiles[x + y * w];
            if (autotile_is_inner_gradient(tiles, w, h, x, y)) {
                t->type |= TILE_TYPE_FLAG_INNER_GRADIENT;
            } else {
                t->type &= ~TILE_TYPE_FLAG_INNER_GRADIENT;
            }
        }
    }
    for (i32 y = y1; y <= y2; y++) {
        for (i32 x = x1; x <= x2; x++) {
            u32 s = (u32)2903735 + (u32)((x + offx) ^ (y + offy)) * 1234807;
            autotile_do_at(tiles, w, h, x, y, &s);
        }
    }
}

void autotile_terrain_section_game_xy(g_s *g, i32 tx1, i32 ty1, i32 tx2, i32 ty2)
{
    autotile_terrain_section_xy(g->tiles, g->tiles_x, g->tiles_y, tx1, ty1, tx2, ty2, 0, 0);
}

void autotile_terrain_section_game(g_s *g, i32 tx, i32 ty, i32 tw, i32 th)
{
    autotile_terrain_section_xy(g->tiles, g->tiles_x, g->tiles_y, tx, ty, tx + tw - 1, ty + th - 1, 0, 0);
}

void autotile_terrain(tile_s *tiles, i32 w, i32 h, i32 offx, i32 offy)
{
    autotile_terrain_section(tiles, w, h, offx, offy, 0, 0, w, h);
}

static bool32 autotile_terrain_is(tile_s *tiles, i32 w, i32 h, i32 x, i32 y, i32 sx, i32 sy)
{
    i32 u = x + sx;
    i32 v = y + sy;
    if (!((u32)u < (u32)w && (u32)v < (u32)h)) return 1;

    tile_s *a = &tiles[x + y * w];
    tile_s *b = &tiles[u + v * w];

    if (a->type & TILE_TYPE_FLAG_INNER_GRADIENT) {
        return (b->type & TILE_TYPE_FLAG_INNER_GRADIENT);
    }

    if (!at_types_blending(tile_get_type(a), tile_get_type(b))) return 0;

    switch (tile_get_shape(b)) {
    case TILE_BLOCK: return 1;
    case TILE_SLOPE_45_0: return ((sx == -1 && sy == +0) ||
                                  (sx == +0 && sy == -1) ||
                                  (sx == -1 && sy == -1));
    case TILE_SLOPE_45_1: return ((sx == -1 && sy == +0) ||
                                  (sx == +0 && sy == +1) ||
                                  (sx == -1 && sy == +1));
    case TILE_SLOPE_45_2: return ((sx == +1 && sy == +0) ||
                                  (sx == +0 && sy == -1) ||
                                  (sx == +1 && sy == -1));
    case TILE_SLOPE_45_3: return ((sx == +1 && sy == +0) ||
                                  (sx == +0 && sy == +1) ||
                                  (sx == +1 && sy == +1));
    default: break;
    }
    return 0;
}

static i32 autotile_marching(tile_s *tiles, i32 w, i32 h, i32 x, i32 y)
{
    if (!((u32)x < (u32)w && (u32)y < (u32)h)) return 0xFF;
    tile_s *tile = &tiles[x + y * w];
    if (tile_get_type(tile) < 3) return 0;

    i32 march = 0;
    if (autotile_terrain_is(tiles, w, h, x, y, +0, -1)) march |= AT_N;
    if (autotile_terrain_is(tiles, w, h, x, y, +1, +0)) march |= AT_E;
    if (autotile_terrain_is(tiles, w, h, x, y, +0, +1)) march |= AT_S;
    if (autotile_terrain_is(tiles, w, h, x, y, -1, +0)) march |= AT_W;
    if (autotile_terrain_is(tiles, w, h, x, y, +1, -1)) march |= AT_NE;
    if (autotile_terrain_is(tiles, w, h, x, y, +1, +1)) march |= AT_SE;
    if (autotile_terrain_is(tiles, w, h, x, y, -1, +1)) march |= AT_SW;
    if (autotile_terrain_is(tiles, w, h, x, y, -1, -1)) march |= AT_NW;
    return march;
}

static bool32 autotile_dual_border(tile_s *tiles, i32 w, i32 h, i32 x, i32 y, i32 sx, i32 sy,
                                   i32 type, i32 march, u32 seed_visuals)
{
    // tile types without dual tiles
    switch (tile_get_type(&tiles[x + y * w])) {
    case TILE_TYPE_DARK_STONE: break;
    default: return 0;
    }

    i32 u = x + sx;
    i32 v = y + sy;
    if (!((u32)u < (u32)w && (u32)v < (u32)h)) return 0;
    tile_s *t = &tiles[u + v * w];
    if (tile_get_type(t) == 6) return 0;

    u32 seed = seed_visuals + ((x | u) + ((y | v)));
    u32 r    = rngs_i32(&seed);
    if (r < 0x8000) return 0;

    // if (t != map_terrain_pack(type, TILE_BLOCK)) return 0;
    return (march == autotile_marching(tiles, w, h, u, v));
}

static void autotile_do_at(tile_s *tiles, i32 w, i32 h, i32 x, i32 y, u32 *seed_visuals)
{
    i32     index = x + y * w;
    tile_s *rtile = &tiles[index];
    if (rtile->type == 0) return;

    i32 rng_v = rngs_i32(seed_visuals);
    rtile->ty = 0;

    i32    m      = autotile_marching(tiles, w, h, x, y);
    v2_i32 tcoord = {0, ((i32)rtile->type) << 3};
    v2_i8  coords = g_autotile_coords[m];
    if (rtile->type & TILE_TYPE_FLAG_INNER_GRADIENT) {
        tcoord.y = TILE_TYPE_ID_BY_XY(0, 6) << 3;
    }

    switch (rtile->shape) {
    case TILE_BLOCK: {
        i32 n_vari = 1; // number of variations in that tileset

        switch (rtile->type) {
        case TILE_TYPE_DARK_STONE:
        case TILE_TYPE_DARK_LEAVES:
        case TILE_TYPE_BRIGHT_STONE:
        case TILE_TYPE_BRIGHT_SNOW:
        case TILE_TYPE_BRIGHT_MOUNTAIN:
        case TILE_TYPE_THORNS: n_vari = 3; break;
        default: break;
        }

        static const v2_i8 altc_17[3] = {{7, 3}, {7, 4}, {7, 6}};
        static const v2_i8 altc_31[3] = {{5, 3}, {0, 4}, {0, 5}};
        static const v2_i8 altc199[3] = {{4, 2}, {1, 6}, {3, 6}};
        static const v2_i8 altc241[3] = {{6, 1}, {6, 3}, {2, 4}};
        static const v2_i8 altc_68[3] = {{3, 7}, {4, 7}, {6, 7}};
        static const v2_i8 altc124[3] = {{4, 0}, {5, 0}, {3, 5}};
        static const v2_i8 altc255[3] = {{4, 1}, {1, 4}, {5, 1}};

        i32 vari = i32_range(rng_v, 0, n_vari - 1);
        switch (m) { // coordinates of variation tiles
        case 17: {   // vertical
            coords = altc_17[vari];
            break;
        }
        case 31: { // left border
            if (0) {
            } else if ((y & 1) == 0 && autotile_dual_border(tiles, w, h, x, y, 0, -1, rtile->type, m, *seed_visuals)) {
                coords.x = 9, coords.y = 5;
            } else if ((y & 1) == 1 && autotile_dual_border(tiles, w, h, x, y, 0, +1, rtile->type, m, *seed_visuals)) {
                coords.x = 8, coords.y = 5;
            } else {
                coords = altc_31[vari];
            }
            break;
        }
        case 199: { // bot border
            if (0) {
            } else if ((x & 1) == 0 && autotile_dual_border(tiles, w, h, x, y, -1, 0, rtile->type, m, *seed_visuals)) {
                coords.x = 9, coords.y = 7;
            } else if ((x & 1) == 1 && autotile_dual_border(tiles, w, h, x, y, +1, 0, rtile->type, m, *seed_visuals)) {
                coords.x = 8, coords.y = 7;
            } else {
                coords = altc199[vari];
            }
            break;
        }
        case 241: { // right border
            if (0) {
            } else if ((y & 1) == 0 && autotile_dual_border(tiles, w, h, x, y, 0, -1, rtile->type, m, *seed_visuals)) {
                coords.x = 9, coords.y = 6;
            } else if ((y & 1) == 1 && autotile_dual_border(tiles, w, h, x, y, 0, +1, rtile->type, m, *seed_visuals)) {
                coords.x = 8, coords.y = 6;
            } else {
                coords = altc241[vari];
            }
            break;
        }
        case 68: { // horizontal
            coords = altc_68[vari];
            break;
        }
        case 124: { // top border
            if (0) {
            } else if ((x & 1) == 0 && autotile_dual_border(tiles, w, h, x, y, -1, 0, rtile->type, m, *seed_visuals)) {
                coords.x = 9, coords.y = 4;
            } else if ((x & 1) == 1 && autotile_dual_border(tiles, w, h, x, y, +1, 0, rtile->type, m, *seed_visuals)) {
                coords.x = 8, coords.y = 4;
            } else {
                coords = altc124[vari];
            }
            break;
        }
        case 255: { // mid
            coords = altc255[0];
            break;
        }
        }

        tcoord.x += coords.x;
        tcoord.y += coords.y;
        break;
    }
    case TILE_SLOPE_45_0:
    case TILE_SLOPE_45_1:
    case TILE_SLOPE_45_2:
    case TILE_SLOPE_45_3: {
        // masks for checking neighbours: SHAPE - X | Y | DIAGONAL
        ALIGNAS(16)
        static const u8 nmasks[4][3] = {{AT_E, AT_S, AT_SE},
                                        {AT_E, AT_N, AT_NE},
                                        {AT_W, AT_S, AT_SW},
                                        {AT_W, AT_N, AT_NW}};
        // Y row of tile
        ALIGNAS(4)
        static const u8 shapei[4] = {0, 2, 1, 3};

        i32 i  = (i32)rtile->shape - TILE_SLOPE_45_0;
        i32 xn = (m & nmasks[i][0]) != 0; // neighbour x
        i32 yn = (m & nmasks[i][1]) != 0; // neighbour y
        i32 cn = (m & nmasks[i][2]) != 0; // neighbour dia

        tcoord.y += shapei[i]; // index shape
        tcoord.x += 8;
        if (xn && yn && cn) { // index variant
            tcoord.x += 3;
        } else if (xn && yn) {
            tcoord.x += 2;
        } else if (yn) {
            tcoord.x += 1;
        }

        break;
    }
    }
    rtile->ty = (i32)tcoord.x + (i32)tcoord.y * 12; // new tile layout
}

static bool32 autotilebg_is(u8 *tiles, i32 w, i32 h, i32 x, i32 y, i32 sx, i32 sy)
{
    i32 u = x + sx;
    i32 v = y + sy;
    if (!((u32)u < (u32)w && (u32)v < (u32)h)) return 1;
    return (0 < tiles[u + v * w]);
}

static void autotilebg_at(g_s *g, u8 *tiles, i32 x, i32 y)
{
    i32 w    = g->tiles_x;
    i32 h    = g->tiles_y;
    i32 i    = x + y * w;
    i32 tile = tiles[i];
    if (tile == 0) return;

    u32 march = 0;
    if (autotilebg_is(tiles, w, h, x, y, +0, -1)) march |= AT_N;
    if (autotilebg_is(tiles, w, h, x, y, +1, +0)) march |= AT_E;
    if (autotilebg_is(tiles, w, h, x, y, +0, +1)) march |= AT_S;
    if (autotilebg_is(tiles, w, h, x, y, -1, +0)) march |= AT_W;
    if (autotilebg_is(tiles, w, h, x, y, +1, -1)) march |= AT_NE;
    if (autotilebg_is(tiles, w, h, x, y, +1, +1)) march |= AT_SE;
    if (autotilebg_is(tiles, w, h, x, y, -1, +1)) march |= AT_SW;
    if (autotilebg_is(tiles, w, h, x, y, -1, -1)) march |= AT_NW;

    v2_i8 coords = g_autotile_coords[march];
    i32   tileID = (i32)coords.x + ((i32)coords.y + tile * 8) * 8;

    g->rtiles[TILELAYER_BG][i] = tileID;
}

void autotilebg(g_s *g, u8 *tiles)
{
    for (i32 y = 0; y < g->tiles_y; y++) {
        for (i32 x = 0; x < g->tiles_x; x++) {
            autotilebg_at(g, tiles, x, y);
        }
    }
}

const v2_i8 g_autotile_coords[256] = {
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {1, 1},
    {0, 2},
    {1, 1},
    {0, 6},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {5, 5},
    {0, 3},
    {5, 5},
    {5, 3},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {1, 1},
    {0, 2},
    {1, 1},
    {0, 6},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {5, 5},
    {0, 3},
    {5, 5},
    {5, 3},
    {7, 0},
    {7, 7},
    {7, 0},
    {7, 7},
    {3, 7},
    {5, 7},
    {3, 7},
    {5, 4},
    {7, 0},
    {7, 7},
    {7, 0},
    {7, 7},
    {3, 7},
    {5, 7},
    {3, 7},
    {5, 4},
    {7, 2},
    {7, 5},
    {7, 2},
    {7, 5},
    {2, 0},
    {2, 1},
    {2, 0},
    {5, 6},
    {7, 2},
    {7, 5},
    {7, 2},
    {7, 5},
    {3, 0},
    {1, 2},
    {3, 0},
    {3, 1},
    {7, 0},
    {7, 7},
    {7, 0},
    {7, 7},
    {3, 7},
    {5, 7},
    {3, 7},
    {5, 4},
    {7, 0},
    {7, 7},
    {7, 0},
    {7, 7},
    {3, 7},
    {5, 7},
    {3, 7},
    {5, 4},
    {4, 3},
    {4, 5},
    {4, 3},
    {4, 5},
    {6, 0},
    {6, 5},
    {6, 0},
    {3, 2},
    {4, 3},
    {4, 5},
    {4, 3},
    {4, 5},
    {4, 0},
    {2, 2},
    {4, 0},
    {1, 3},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {1, 1},
    {0, 2},
    {1, 1},
    {0, 6},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {5, 5},
    {0, 3},
    {5, 5},
    {5, 3},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {1, 1},
    {0, 2},
    {1, 1},
    {0, 6},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {5, 5},
    {0, 3},
    {5, 5},
    {5, 3},
    {7, 0},
    {6, 6},
    {7, 0},
    {6, 6},
    {3, 7},
    {4, 6},
    {3, 7},
    {4, 2},
    {7, 0},
    {6, 6},
    {7, 0},
    {6, 6},
    {3, 7},
    {4, 6},
    {3, 7},
    {4, 2},
    {7, 2},
    {6, 4},
    {7, 2},
    {6, 4},
    {2, 0},
    {4, 4},
    {2, 0},
    {2, 6},
    {7, 2},
    {6, 4},
    {7, 2},
    {6, 4},
    {3, 0},
    {3, 3},
    {3, 0},
    {5, 2},
    {7, 0},
    {6, 6},
    {7, 0},
    {6, 6},
    {3, 7},
    {4, 6},
    {3, 7},
    {4, 2},
    {7, 0},
    {6, 6},
    {7, 0},
    {6, 6},
    {3, 7},
    {4, 6},
    {3, 7},
    {4, 2},
    {4, 3},
    {6, 1},
    {4, 3},
    {6, 1},
    {6, 0},
    {6, 2},
    {6, 0},
    {2, 3},
    {4, 3},
    {6, 1},
    {4, 3},
    {6, 1},
    {4, 0},
    {2, 5},
    {4, 0},
    {4, 1}};