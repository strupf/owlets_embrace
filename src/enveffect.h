// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _ENVEFFECT_H
#define _ENVEFFECT_H

#include "gamedef.h"

#define BG_WIND_CIRCLE_R   1900
#define BG_NUM_PARTICLES   512
#define BG_NUM_CLOUD_TYPES 3
#define BG_WIND_PARTICLE_N 8
#define BG_SIZE            512

enum {
    ENVEFFECT_WIND = 1 << 0,
    ENVEFFECT_HEAT = 1 << 1,
};

typedef struct {
    i32    i;
    i32    ticks;
    i32    circticks;
    i32    circcooldown;
    v2_i32 p_q8; // q8
    v2_i32 v_q8;
    v2_i32 pos_q8[BG_WIND_PARTICLE_N];
    v2_i32 circc;
} windparticle_s;

typedef struct {
    int            n;
    windparticle_s p[BG_NUM_PARTICLES];
} enveffect_wind_s;

#define BG_NUM_HEAT_ROWS 256

typedef struct {
    i8  offset[BG_NUM_HEAT_ROWS];
    int tick;
} enveffect_heat_s;

void enveffect_wind_update(enveffect_wind_s *e);
void enveffect_wind_draw(gfx_ctx_s ctx, enveffect_wind_s *e, v2_i32 cam);

void enveffect_heat_update(enveffect_heat_s *e);
void enveffect_heat_draw(gfx_ctx_s ctx, enveffect_heat_s *e, v2_i32 cam);

void backforeground_animate_grass(game_s *g);

#endif