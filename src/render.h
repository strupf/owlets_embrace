// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef RENDER_H
#define RENDER_H

#include "gamedef.h"

void render_tilemap(game_s *g, int layer, bounds_2D_s bounds, v2_i32 offset);
void render_parallax(game_s *g, v2_i32 offset);
void render_ui(game_s *g, v2_i32 camoffset);
void render_pause(game_s *g);

#endif