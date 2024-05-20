// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef TILE_H
#define TILE_H

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
    TILE_SLOPE_LO_0,
    TILE_SLOPE_LO_1,
    TILE_SLOPE_LO_2,
    TILE_SLOPE_LO_3,
    TILE_SLOPE_LO_4,
    TILE_SLOPE_LO_5,
    TILE_SLOPE_LO_6,
    TILE_SLOPE_LO_7,
    //
    TILE_SLOPE_HI_0,
    TILE_SLOPE_HI_1,
    TILE_SLOPE_HI_2,
    TILE_SLOPE_HI_3,
    TILE_SLOPE_HI_4,
    TILE_SLOPE_HI_5,
    TILE_SLOPE_HI_6,
    TILE_SLOPE_HI_7,
    //
    NUM_TILE_SHAPES,
    //
    TILE_LADDER = NUM_TILE_SHAPES,
    TILE_LADDER_ONE_WAY,
    TILE_ONE_WAY,
    TILE_SPIKES,
};

#define TILE_IS_BLOCK(X)    (TILE_BLOCK == (X))
#define TILE_IS_SLOPE_45(X) (TILE_SLOPE_45_0 <= (X) && (X) <= TILE_SLOPE_45_3)
#define TILE_IS_SLOPE_LO(X) (TILE_SLOPE_LO_0 <= (X) && (X) <= TILE_SLOPE_LO_7)
#define TILE_IS_SLOPE_HI(X) (TILE_SLOPE_HI_0 <= (X) && (X) <= TILE_SLOPE_HI_7)
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

tile_walls_s tile_walls_get(i32 ID);
tile_tris_s  tile_tris_get(i32 ID);

// operate on 16x16 tiles
// these are the collision masks per pixel row of a tile

extern const tile_corners_s g_tile_corners[NUM_TILE_SHAPES];
extern const u16            g_tile_px[NUM_TILE_SHAPES][16];
extern const i32            g_tile_tris[NUM_TILE_SHAPES * 12];

#endif