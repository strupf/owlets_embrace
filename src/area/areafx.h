// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _AREAFX_H
#define _AREAFX_H

#include "core/gfx.h"
#include "gamedef.h"

enum {
    AFX_CLOUDS         = 1 << 0,
    AFX_RAIN           = 1 << 1,
    AFX_WIND           = 1 << 2,
    AFX_HEAT           = 1 << 3,
    AFX_LEAVES         = 1 << 4,
    AFX_PARTICLES_CALM = 1 << 5,
};

#define AREAFX_CLOUDS_TYPES 3
#define AREAFX_CLOUDS       64

typedef struct {
    texrec_s t;
    v2_i32   p;
    i32      mx_q8;
    i32      vx_q8;
    i32      priority;
} areafx_cloud_s;

typedef struct {
    bool32         dirty;
    i32            n;
    areafx_cloud_s clouds[AREAFX_CLOUDS];
} areafx_clouds_s;

#define AREAFX_WIND_SIZEY 512
#define AREAFX_WIND_R     1900
#define AREAFX_WINDPT     512
#define AREAFX_WINDPT_N   8

typedef struct {
    i32    i;
    i32    ticks;
    i32    circticks;
    i32    circcooldown;
    v2_i32 p_q8; // q8
    v2_i32 v_q8;
    v2_i32 pos_q8[AREAFX_WINDPT_N];
    v2_i32 circc;
} areafx_windpt_s;

typedef struct {
    int             n;
    areafx_windpt_s p[AREAFX_WINDPT];
} areafx_wind_s;

#define AREAFX_HEAT_ROWS PLTF_DISPLAY_H

typedef struct {
    i32 tick;
    i8  offset[AREAFX_HEAT_ROWS];
} areafx_heat_s;

#define AREAFX_RAIN_DROPS 512

typedef struct {
    v2_i32 p;
    v2_i32 v;
} areafx_raindrop_s;

typedef struct {
    i32 lightning_tick;
    i32 lightning_twice;

    i32               n_drops;
    areafx_raindrop_s drops[AREAFX_RAIN_DROPS];
} areafx_rain_s;

typedef struct {
    i32 x;
} areafx_leaves_s;

#define AREAFX_PT_CALM_N 96
#define PT_CALM_VRNG     16
#define PT_CALM_VCAP     128
#define PT_CALM_X_RANGE  512
#define PT_CALM_Y_RANGE  512

typedef struct {
    v2_i32 pos;
    v2_i32 vel;
} areafx_particle_calm_s;

typedef struct {
    areafx_particle_calm_s p[AREAFX_PT_CALM_N];
} areafx_particles_calm_s;

void areafx_clouds_setup(game_s *g, areafx_clouds_s *fx);
void areafx_clouds_update(game_s *g, areafx_clouds_s *fx);
void areafx_clouds_draw(game_s *g, areafx_clouds_s *fx, v2_i32 cam);
//
void areafx_rain_setup(game_s *g, areafx_rain_s *fx);
void areafx_rain_update(game_s *g, areafx_rain_s *fx);
void areafx_rain_draw(game_s *g, areafx_rain_s *fx, v2_i32 cam);
void areafx_rain_draw_lightning(game_s *g, areafx_rain_s *fx, v2_i32 cam);
//
void areafx_wind_setup(game_s *g, areafx_wind_s *fx);
void areafx_wind_update(game_s *g, areafx_wind_s *fx);
void areafx_wind_draw(game_s *g, areafx_wind_s *fx, v2_i32 cam);
//
void areafx_heat_setup(game_s *g, areafx_heat_s *fx);
void areafx_heat_update(game_s *g, areafx_heat_s *fx);
void areafx_heat_draw(game_s *g, areafx_heat_s *fx, v2_i32 cam);
//
void areafx_leaves_setup(game_s *g, areafx_leaves_s *fx);
void areafx_leaves_update(game_s *g, areafx_leaves_s *fx);
void areafx_leaves_draw(game_s *g, areafx_leaves_s *fx, v2_i32 cam);
//
void areafx_particles_calm_setup(game_s *g, areafx_particles_calm_s *fx);
void areafx_particles_calm_update(game_s *g, areafx_particles_calm_s *fx);
void areafx_particles_calm_draw(game_s *g, areafx_particles_calm_s *fx, v2_i32 cam);

#endif