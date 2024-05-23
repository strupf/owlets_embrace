// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef LIGHTING_H_
#define LIGHTING_H_

#include "gamedef.h"

#define LIGHTING_ENABLED    0
#define NUM_LIGHTING_LIGHTS 16

#define LIGHTING_W 420
#define LIGHTING_H 260

#define LIGHTING_W2 (416 >> 3)
#define LIGHTING_H2 (256 >> 3)

typedef struct {
    v2_i32 p;
    i32    r;
} l_light_s;

typedef struct {
    i32       n_lights;
    l_light_s lights[NUM_LIGHTING_LIGHTS];

    u8 l3[LIGHTING_W2 * LIGHTING_W2];
    u8 l2[LIGHTING_W * LIGHTING_H];
    u8 l[LIGHTING_W * LIGHTING_H];
} lighting_s;

void lighting_init(lighting_s *lig);
void lighting_do(game_s *g, lighting_s *lig, v2_i32 cam);

#endif