// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TILE_MAP_H
#define TILE_MAP_H

#include "gamedef.h"

enum {
    TILE_EMPTY,
    TILE_BLOCK,
    //
    TILE_SLOPE_45_0,
    TILE_SLOPE_45_1,
    TILE_SLOPE_45_2,
    TILE_SLOPE_45_3,
    //
    NUM_TILE_SHAPES,
    //
    TILE_LADDER = NUM_TILE_SHAPES,
    TILE_LADDER_ONE_WAY,
    TILE_ONE_WAY,
    TILE_CLIMBWALL,
    TILE_CLIMBWALL_ONEWAY,
    TILE_SPIKES,
};

enum {
    TILELAYER_BG,
    TILELAYER_PROP_BG,
    TILELAYER_PROP_FG,
    //
    NUM_TILELAYER
};

enum {
    TILE_TYPE_NONE,
    TILE_TYPE_INVISIBLE_NON_CONNECTING,
    TILE_TYPE_INVISIBLE_CONNECTING,
    TILE_TYPE_DEBUG,
    TILE_TYPE_THORNS,
    TILE_TYPE_SPIKES,
    TILE_TYPE_______1,
    TILE_TYPE_______2,
    TILE_TYPE_DIRT,
    TILE_TYPE_LEAVES,
    TILE_TYPE_STONE_SQUARE_LIGHT,
    TILE_TYPE_______3,
    TILE_TYPE_______4,
    TILE_TYPE_______5,
    TILE_TYPE_______6,
    TILE_TYPE_STONE_ROUND_DARK,
    TILE_TYPE_STONE_SQUARE_DARK,
    //
    NUM_TILE_TYPES = 24
};

static const u8 tile_type_connects_to[] = {
    //

    /* */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static inline i32 tile_type_render_priority(i32 type)
{
    return type;
}

#define TILE_WATER_MASK     0x80
#define TILE_ICE_MASK       0x40
#define TILE_IS_BLOCK(X)    (TILE_BLOCK == (X))
#define TILE_IS_SLOPE_45(X) (TILE_SLOPE_45_0 <= (X) && (X) <= TILE_SLOPE_45_3)
#define TILE_IS_SHAPE(X)    (1 <= (X) && (X) < NUM_TILE_SHAPES)

typedef struct {
    i32   n;
    v2_i8 c[4];
} tile_corners_s;

typedef struct {
    v2_i32 a;
    v2_i32 b;
    v2_i32 nor_q8; // normal vector pointing away in Q8
} tile_wall_s;

typedef struct {
    i32         n;
    tile_wall_s w[4];
} tile_walls_s;

typedef struct {
    i32     n;
    tri_i32 t[2];
} tile_tris_s;

typedef union {
    struct {
        union {
            struct {
                u8 tx; // autotile x
                u8 ty; // autotile y
            };
            u16 u;
        };
        u8 type;      // 6 bits for type, 1 bit for water, 1 bit for iced
        u8 collision; // collision shape
    };
    u32 U;
} tile_s;

typedef union {
    struct {
        u8 tx;
        u8 ty;
    };
    u16 u;
} rtile_s;

bool32       tile_solid_pt(i32 ID, i32 x, i32 y);
bool32       tile_solid_r(i32 ID, i32 x0, i32 y0, i32 x1, i32 y1);
tile_walls_s tile_walls_get(i32 ID);
tile_tris_s  tile_tris_get(i32 ID);
//
bool32       tile_map_hookable(g_s *g, rec_i32 r);
bool32       tile_map_solidr(g_s *g, i32 x, i32 y, i32 w, i32 h);
bool32       tile_map_solido(g_s *g, obj_s *o, i32 dx, i32 dy);
bool32       tile_map_solid(g_s *g, rec_i32 r);
bool32       tile_map_solid_pt(g_s *g, i32 x, i32 y);
bool32       tile_map_ladder_overlaps_rec(g_s *g, rec_i32 r, v2_i32 *tpos);
void         tile_map_set_collision(g_s *g, rec_i32 r, i32 shape, i32 type);
//
bool32       map_blocked_by_solid(g_s *g, obj_s *o, rec_i32 r, i32 m);
bool32       map_blocked_by_any_solid(g_s *g, rec_i32 r);
bool32       map_blocked_by_any_solid_pt(g_s *g, i32 x, i32 y);
bool32       map_blocked_by_any_climbable_pt(g_s *g, i32 x, i32 y);
bool32       map_traversable(g_s *g, rec_i32 r);
bool32       map_traversable_pt(g_s *g, i32 x, i32 y);
bool32       map_climbable_pt(g_s *g, i32 x, i32 y);

extern const tile_corners_s g_tile_corners[NUM_TILE_SHAPES];
extern const i32            g_tile_tris[NUM_TILE_SHAPES * 12];

typedef struct {
    i32 x1, y1, x2, y2;
} tile_map_bounds_s;

tile_map_bounds_s tile_map_bounds_rec(g_s *g, rec_i32 r);
tile_map_bounds_s tile_map_bounds_pts(g_s *g, v2_i32 p0, v2_i32 p1);
tile_map_bounds_s tile_map_bounds_tri(g_s *g, tri_i32 t);

#endif