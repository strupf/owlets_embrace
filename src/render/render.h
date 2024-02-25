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

void render_tilemap(game_s *g, int layer, bounds_2D_s bounds, v2_i32 offset);
void render_water_and_terrain(game_s *g, bounds_2D_s bounds, v2_i32 offset);
void render_ui(game_s *g, v2_i32 camoff);
void prerender_area_label(game_s *g);
void render_pause(game_s *g);
void ocean_calc_spans(game_s *g, rec_i32 camr);
void render_rec_as_terrain(gfx_ctx_s ctx, rec_i32 r, int terrain);
void render_water_background(game_s *g, v2_i32 camoff, bounds_2D_s bounds);

#endif