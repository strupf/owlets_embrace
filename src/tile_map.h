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
    TILELAYER_BG_TILE,
    TILELAYER_PROP_BG,
    TILELAYER_PROP_FG,
    //
    NUM_TILELAYER
};

enum {
    TILE_TYPE_NONE                     = 0,
    TILE_TYPE_INVISIBLE_NON_CONNECTING = 1,
    TILE_TYPE_INVISIBLE_CONNECTING     = 2,
    //
    TILE_TYPE_DARK_BEG                 = 3,
    TILE_TYPE_DARK_LEAVES              = 3,
    TILE_TYPE_DARK_STONE               = 4,
    TILE_TYPE_DARK_STONE_PEBBLE        = 5,
    TILE_TYPE_DARK_OBSIDIAN            = 6,
    TILE_TYPE_DARK_2                   = 7,
    TILE_TYPE_DARK_3                   = 8,
    TILE_TYPE_DARK_4                   = 9,
    TILE_TYPE_DARK_5                   = 10,
    TILE_TYPE_DARK_END                 = 10,
    //
    TILE_TYPE_BRIGHT_BEG               = 11,
    TILE_TYPE_BRIGHT_STONE             = 11,
    TILE_TYPE_BRIGHT_SNOW              = 12,
    TILE_TYPE_BRIGHT_BREAKING          = 13,
    TILE_TYPE_BRIGHT_END               = 18,
    //
    TILE_TYPE_THORNS                   = 19,
    TILE_TYPE_THORNS1                  = 20,
    TILE_TYPE_THORNS2                  = 21,
    TILE_TYPE_CRUMBLE                  = 22,
    //
    NUM_TILE_TYPES                     = 24
};

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
    i32     n;
    tri_i32 t[2];
} tile_tris_s;

typedef union {
    struct {
        u16 ty;
        u8  type;      // 6 bits for type, 1 bit for water, 1 bit for iced
        u8  collision; // collision shape
    };
    u32 u;
} tile_s;

bool32  tile_solid_pt(i32 shape, i32 x, i32 y);
bool32  tile_solid_r(i32 shape, i32 x0, i32 y0, i32 x1, i32 y1);
//
bool32  tile_map_hookable(g_s *g, rec_i32 r);
bool32  tile_map_solid(g_s *g, rec_i32 r);
bool32  tile_map_solid_pt(g_s *g, i32 x, i32 y);
void    tile_map_set_collision(g_s *g, rec_i32 r, i32 shape, i32 type);
tile_s *tile_map_at_pos(g_s *g, v2_i32 p);
//
bool32  map_blocked_excl(g_s *g, rec_i32 r, obj_s *o);
bool32  map_blocked(g_s *g, rec_i32 r);
bool32  map_blocked_excl_offs(g_s *g, rec_i32 r, obj_s *o, i32 dx, i32 dy);
bool32  map_blocked_offs(g_s *g, rec_i32 r, i32 dx, i32 dy);
bool32  map_blocked_pt(g_s *g, i32 x, i32 y);

enum {
    MAP_CLIMBABLE_NO_TERRAIN,
    MAP_CLIMBABLE_SUCCESS,
    MAP_CLIMBABLE_SLIPPERY,
};

i32 map_climbable_pt(g_s *g, i32 x, i32 y);

extern const i32     g_tile_tris[NUM_TILE_SHAPES * 12];
extern const tri_i16 g_tiletris[NUM_TILE_SHAPES];

typedef struct {
    i32 x1, y1, x2, y2;
} tile_map_bounds_s;

tile_map_bounds_s tile_map_bounds_rec(g_s *g, rec_i32 r);
tile_map_bounds_s tile_map_bounds_pts(g_s *g, v2_i32 p0, v2_i32 p1);
tile_map_bounds_s tile_map_bounds_tri(g_s *g, tri_i32 t);

#endif