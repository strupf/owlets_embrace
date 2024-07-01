// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
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
void   render_map(game_s *g, gfx_ctx_s ctx, i32 x, i32 y, i32 w, i32 h, i32 s_q8, v2_i32 c_q8);
void   render_tilemap(game_s *g, int layer, tile_map_bounds_s bounds, v2_i32 offset);
void   render_water_and_terrain(game_s *g, tile_map_bounds_s bounds, v2_i32 offset);
void   render_ui(game_s *g, v2_i32 camoff);
void   render_jump_ui(game_s *g, obj_s *o, v2_i32 camoff);
void   prerender_area_label(game_s *g);
void   render_pause(game_s *g);
void   ocean_calc_spans(game_s *g, rec_i32 camr);
void   render_rec_as_terrain(gfx_ctx_s ctx, rec_i32 r, int terrain);
void   render_water_background(game_s *g, v2_i32 camoff, tile_map_bounds_s bounds);
v2_i32 parallax_offs(v2_i32 cam, v2_i32 pos, i32 x_q8, i32 y_q8);

#endif