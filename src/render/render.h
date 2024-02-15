// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef RENDER_H
#define RENDER_H

#include "gamedef.h"

void render_tilemap(game_s *g, int layer, bounds_2D_s bounds, v2_i32 offset);
void render_terraintiles(game_s *g, bounds_2D_s bounds, v2_i32 offset);
void render_ui(game_s *g, v2_i32 camoff);
void prerender_area_label(game_s *g);
void render_pause(game_s *g);
void render_bg(game_s *g, rec_i32 cam, bounds_2D_s bounds);

void ocean_calc_spans(game_s *g, rec_i32 camr);
void ocean_draw_bg(gfx_ctx_s ctx, game_s *g, v2_i32 camoff);
void ocean_draw(game_s *g, tex_s t, v2_i32 camoff);

#endif