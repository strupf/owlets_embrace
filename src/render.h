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

v2_i32 mapview_world_q8_from_screen(v2_i32 pxpos, v2_i32 ctr_wpos_q8,
                                    i32 w, i32 h, i32 scl_q8);
v2_i32 mapview_screen_from_world_q8(v2_i32 wpos_q8, v2_i32 ctr_wpos_q8,
                                    i32 w, i32 h, i32 scl_q8);
void   render_map(g_s *g, gfx_ctx_s ctx, i32 x, i32 y, i32 w, i32 h, i32 s_q8, v2_i32 c_q8);
void   render_tilemap(g_s *g, int layer, tile_map_bounds_s bounds, v2_i32 offset);
void   render_terrain(g_s *g, tile_map_bounds_s bounds, v2_i32 offset);
void   render_ui(g_s *g, v2_i32 camoff);
void   render_stamina_ui(g_s *g, obj_s *o, v2_i32 camoff);
void   prerender_area_label(g_s *g);
void   render_pause(g_s *g);
void   render_fluids(g_s *g, v2_i32 camoff, tile_map_bounds_s bounds);
v2_i32 parallax_offs(v2_i32 cam, v2_i32 pos, i32 x_q8, i32 y_q8);

// gets the tile index of a terrain block
// tx = width of block in tiles
// ty = height of block in tiles
// x = tile position within tx
// y = tile position within ty
// returns index into "transformed" tile coordinates
i32  tileindex_terrain_block(i32 tx, i32 ty, i32 tile_type, i32 x, i32 y);
void render_tile_terrain_block(gfx_ctx_s ctx, v2_i32 pos, i32 tx, i32 ty, i32 tile_type);

#endif