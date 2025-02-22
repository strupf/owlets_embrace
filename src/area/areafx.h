// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _AREAFX_H
#define _AREAFX_H

#include "core/gfx.h"
#include "gamedef.h"

enum {
    AFX_NONE,
    AFX_RAIN,
    AFX_SNOW,
    AFX_WIND,
    AFX_HEAT,
    AFX_PARTICLES_CALM,
};

enum {
    AREAFX_SNOW_NO_WIND,
    AREAFX_SNOW_MODERATE,
    AREAFX_SNOW_STORM
};

#define AREAFX_NUM_SNOWFLAKES 256
#define AREAFX_SNOW_W         512
#define AREAFX_SNOW_H         256

typedef struct {
    v2_i16 p_q4;
    v2_i16 v_q4;
} areafx_snowflake_s;

typedef struct {
    i32                type;
    i32                n;
    areafx_snowflake_s snowflakes[AREAFX_NUM_SNOWFLAKES];
} areafx_snow_s;

void areafx_snow_setup(g_s *g, areafx_snow_s *fx);
void areafx_snow_update(g_s *g, areafx_snow_s *fx);
void areafx_snow_draw(g_s *g, areafx_snow_s *fx, v2_i32 cam);

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
    i32             n;
    areafx_windpt_s p[AREAFX_WINDPT];
} areafx_wind_s;

void areafx_wind_setup(g_s *g, areafx_wind_s *fx);
void areafx_wind_update(g_s *g, areafx_wind_s *fx);
void areafx_wind_draw(g_s *g, areafx_wind_s *fx, v2_i32 cam);

typedef struct {
    i32 tick;
} areafx_heat_s;

void areafx_heat_setup(g_s *g, areafx_heat_s *fx);
void areafx_heat_update(g_s *g, areafx_heat_s *fx);
void areafx_heat_draw(g_s *g, areafx_heat_s *fx, v2_i32 cam);

#define AREAFX_RAIN_DROPS 512
#define AREAFX_RAIN_W     512
#define AREAFX_RAIN_H     256

typedef struct {
    v2_i16 p;
    v2_i16 v;
} areafx_raindrop_s;

typedef struct {
    i32               lightning_tick;
    i32               lightning_twice;
    i32               n_drops;
    areafx_raindrop_s drops[AREAFX_RAIN_DROPS];
} areafx_rain_s;

void areafx_rain_setup(g_s *g, areafx_rain_s *fx);
void areafx_rain_update(g_s *g, areafx_rain_s *fx);
void areafx_rain_draw(g_s *g, areafx_rain_s *fx, v2_i32 cam);
void areafx_rain_draw_lightning(g_s *g, areafx_rain_s *fx, v2_i32 cam);

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

void areafx_particles_calm_setup(g_s *g, areafx_particles_calm_s *fx);
void areafx_particles_calm_update(g_s *g, areafx_particles_calm_s *fx);
void areafx_particles_calm_draw(g_s *g, areafx_particles_calm_s *fx, v2_i32 cam);

#endif