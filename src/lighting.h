// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _LIGHTING_H
#define _LIGHTING_H

#include "gamedef.h"

#define NUM_LIGHTS 16

typedef struct {
    v2_i32 a;
    v2_i32 b;
} wall_s;

typedef struct light_s light_s;
struct light_s {
    v2_i32 p;
    i32    r;
    tex_s  t;
};

void lighting_do(tex_s tex, light_s *lights, int n_l, wall_s *walls, int n_w);

#endif