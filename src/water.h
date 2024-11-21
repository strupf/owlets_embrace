// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef WATER_H
#define WATER_H

#include "gamedef.h"

#define NUM_WATER       16
#define OCEAN_W_WORDS   PLTF_DISPLAY_WWORDS
#define OCEAN_W_PX      PLTF_DISPLAY_W
#define OCEAN_H_PX      PLTF_DISPLAY_H
#define OCEAN_NUM_SPANS 512

typedef struct {
    i16 y;
    i16 w;
} ocean_span_s;

typedef struct {
    bool32       active;
    i32          y;
    i32          y_min; // boundaries of affected wave area
    i32          y_max;
    i32          n_spans;
    ocean_span_s spans[OCEAN_NUM_SPANS];
} ocean_s;

void water_prerender_tiles();
i32  water_tile_get(i32 x, i32 y, i32 tick);
i32  water_depth_rec(g_s *g, rec_i32 r);
i32  ocean_height(g_s *g, i32 pixel_x);
i32  ocean_render_height(g_s *g, i32 pixel_x);

#endif