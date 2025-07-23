// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TILE_MAP_H
#define TILE_MAP_H

#include "gamedef.h"
#include "tile_types.h"

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
    TILELAYER_FG,
    //
    NUM_TILELAYER
};

#define TILE_IS_BLOCK(X)    (TILE_BLOCK == (X))
#define TILE_IS_SLOPE_45(X) (TILE_SLOPE_45_0 <= (X) && (X) <= TILE_SLOPE_45_3)
#define TILE_IS_SHAPE(X)    (1 <= (X) && (X) < NUM_TILE_SHAPES)

typedef struct {
    i32     n;
    tri_i32 t[2];
} tile_tris_s;

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

// bits for marching squares (neighbours)
// 128  1 2
//  64 XX 4
//  32 16 8
enum {
    AT_N  = B8(00000001),
    AT_E  = B8(00000100),
    AT_S  = B8(00010000),
    AT_W  = B8(01000000),
    AT_NE = B8(00000010),
    AT_SE = B8(00001000),
    AT_SW = B8(00100000),
    AT_NW = B8(10000000)
};

// offx and offy only affect the visual tile randomizer
void autotile_terrain_section(tile_s *tiles, i32 w, i32 h, i32 offx, i32 offy,
                              i32 rx, i32 ry, i32 rw, i32 rh);
// offx and offy only affect the visual tile randomizer
void autotile_terrain(tile_s *tiles, i32 w, i32 h, i32 offx, i32 offy);

// convert u8 array to background autotiles
void               autotilebg(g_s *g, u8 *tiles);
extern const v2_i8 g_autotile_coords[256];

#endif