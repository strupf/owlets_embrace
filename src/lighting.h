// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef LIGHTING_H_
#define LIGHTING_H_

#include "gamedef.h"

#define LIGHTING_ENABLED    0
#define NUM_LIGHTING_LIGHTS 8

typedef struct {
    v2_i32 p;
    i32    r;
    i32    mm;
    i32    w;
    u8     l[256 * 256];
    tex_s  tex;
} l_light_s;

typedef struct {
    i32       n_lights;
    u8        l[(416 >> 3) * (256 >> 3)];
    l_light_s lights[NUM_LIGHTING_LIGHTS];
    tex_s     merged;
} lighting_s;

void lighting_init(lighting_s *lig);
void lighting_refresh(game_s *g, lighting_s *lig);
void lighting_render(lighting_s *lig, v2_i32 cam);
void lighting_apply_tex(lighting_s *lig);
void lighting_shadowcast(game_s *g, lighting_s *lig, v2_i32 cam);

#endif