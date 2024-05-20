// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef LIGHTING_H
#define LIGHTING_H

#include "gamedef.h"

#define NUM_LIGHTING_LIGHTS 16

typedef struct {
    v2_i32 a;
    v2_i32 b;
} l_wall_s;

typedef struct {
    v2_i32 p;
    i32    r;
} l_light_s;

typedef struct {
    tex_s     tex;
    i32       n_lights;
    l_light_s lights[NUM_LIGHTING_LIGHTS];
    u8        l[SYS_DISPLAY_W * SYS_DISPLAY_H / 4];
} lighting_s;

void lighting_init(lighting_s *lig);
void lighting_do(game_s *g, lighting_s *lig, v2_i32 cam);

#endif