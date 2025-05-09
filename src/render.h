// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef RENDER_H
#define RENDER_H

enum {
    SCENE_MOUNTAINS,
    SCENE_CAVE,
    SCENE_DEEP_FOREST,
    SCENE_FOREST,
    SCENE_MAGMA,
};

#include "gamedef.h"

void   render_tilemap(g_s *g, int layer, tile_map_bounds_s bounds, v2_i32 offset);
void   render_terrain(g_s *g, tile_map_bounds_s bounds, v2_i32 offset);
void   render_ui(g_s *g, v2_i32 camoff);
void   render_stamina_ui(g_s *g, obj_s *o, v2_i32 camoff);
void   prerender_area_label(g_s *g);
void   render_pause(g_s *g);
void   render_fluids(g_s *g, v2_i32 camoff, tile_map_bounds_s bounds);
void   render_map_doors(g_s *g, v2_i32 camoff);
v2_i32 parallax_offs(v2_i32 cam, v2_i32 pos, i32 x_q8, i32 y_q8);

// gets the tile index of a terrain block
// tx = width of block in tiles
// ty = height of block in tiles
// x = tile position within tx
// y = tile position within ty
// returns index into "transformed" tile coordinates
i32  tileindex_terrain_block(i32 tx, i32 ty, i32 tile_type, i32 x, i32 y);
void render_tile_terrain_block(gfx_ctx_s ctx, v2_i32 pos, i32 tx, i32 ty, i32 tile_type);

void render_map_transition_in(g_s *g, v2_i32 cam, i32 t, i32 t2);
void render_map_transition_out(g_s *g, v2_i32 cam, i32 t, i32 t2);

#endif