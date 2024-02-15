// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _ENVEFFECT_H
#define _ENVEFFECT_H

#include "gamedef.h"

#define BG_WIND_CIRCLE_R    1900
#define BG_NUM_PARTICLES    512
#define BG_NUM_CLOUD_TYPES  3
#define BG_NUM_CLOUDS       64
#define BG_WIND_PARTICLE_N  8
#define BG_SIZE             512
#define BG_NUM_RAINDROPS    512
#define BG_NUM_RAINSPLASHES 512

enum {
    ENVEFFECT_WIND   = 1 << 0,
    ENVEFFECT_HEAT   = 1 << 1,
    ENVEFFECT_CLOUD  = 1 << 2,
    ENVEFFECT_LEAVES = 1 << 3,
    ENVEFFECT_RAIN   = 1 << 4,
};

typedef struct {
    texrec_s t;
    v2_i32   p;
    i32      mx_q8;
    i32      vx_q8;
    i32      priority;
} cloud_s;

typedef struct {
    bool32  dirty;
    int     n;
    cloud_s clouds[BG_NUM_CLOUDS];
} enveffect_cloud_s;

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

typedef struct {
    int tick;
} enveffect_leaves_s;

typedef struct {
    v2_i32 p;
    v2_i32 v;
} raindrop_s;

typedef struct {
    int    tick;
    v2_i32 pos;
} rainsplash_s;

typedef struct {
    int lightning_tick;
    int lightning_twice;

    int        n_drops;
    raindrop_s drops[BG_NUM_RAINDROPS];

    int          n_splashes;
    rainsplash_s splashes[BG_NUM_RAINSPLASHES];
} enveffect_rain_s;

void enveffect_cloud_setup(enveffect_cloud_s *e);
void enveffect_cloud_update(enveffect_cloud_s *e);
void enveffect_cloud_draw(gfx_ctx_s ctx, enveffect_cloud_s *e, v2_i32 cam);
//
void enveffect_wind_update(enveffect_wind_s *e);
void enveffect_wind_draw(gfx_ctx_s ctx, enveffect_wind_s *e, v2_i32 cam);
//
void enveffect_leaves_setup(enveffect_leaves_s *e);
void enveffect_leaves_update(enveffect_leaves_s *e);
void enveffect_leaves_draw(gfx_ctx_s ctx, enveffect_leaves_s *e, v2_i32 cam);
//
void enveffect_rain_setup(enveffect_rain_s *e);
void enveffect_rain_update(game_s *g, enveffect_rain_s *e);
void enveffect_rain_draw(gfx_ctx_s ctx, enveffect_rain_s *e, v2_i32 cam);
//
void enveffect_heat_update(enveffect_heat_s *e);
void enveffect_heat_draw(gfx_ctx_s ctx, enveffect_heat_s *e, v2_i32 cam);

void backforeground_animate_grass(game_s *g);

#endif