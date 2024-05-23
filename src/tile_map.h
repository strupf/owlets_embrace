// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TILE_MAP_H
#define TILE_MAP_H

#include "gamedef.h"

#define NUM_TILES        (2 * 131072)
#define NUM_STATIC_WALLS (NUM_TILES)

#define TILE_WATER_MASK 0x80
#define TILE_ICE_MASK   0x40

enum {
    TILELAYER_BG,
    TILELAYER_PROP_BG,
    TILELAYER_PROP_FG,
    //
    NUM_TILELAYER
};

enum {
    TILE_TYPE_NONE,
    TILE_TYPE_FAKE_1,
    TILE_TYPE_FAKE_2,
    //
    TILE_TYPE_CLEAN       = 3,
    TILE_TYPE_BRICK       = 4,
    TILE_TYPE_BRICK_SMALL = 5,
    TILE_TYPE_DIRT        = 6,
    TILE_TYPE_STONE       = 7,
    TILE_TYPE_1           = 8,
    TILE_TYPE_2           = 9,
    TILE_TYPE_3           = 10,
    TILE_TYPE_DIRT_DARK   = 11,
    //
    NUM_TILE_TYPES
};

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

bool32 tile_map_hookable(game_s *g, rec_i32 r);
bool32 tile_map_solid(game_s *g, rec_i32 r);
bool32 tile_map_solid_pt(game_s *g, i32 x, i32 y);
bool32 tile_map_one_way(game_s *g, rec_i32 r);
bool32 tile_map_ladder_overlaps_rec(game_s *g, rec_i32 r, v2_i32 *tpos);
void   tile_map_set_collision(game_s *g, rec_i32 r, i32 shape, i32 type);

typedef struct {
    i32 x1, y1, x2, y2;
} tile_map_bounds_s;

tile_map_bounds_s tile_map_bounds_rec(game_s *g, rec_i32 r);
tile_map_bounds_s tile_map_bounds_pts(game_s *g, v2_i32 p0, v2_i32 p1);
tile_map_bounds_s tile_map_bounds_tri(game_s *g, tri_i32 t);

#endif