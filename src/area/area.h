// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _AREA_H
#define _AREA_H

#include "areafx.h"
#include "core/gfx.h"
#include "gamedef.h"

enum {
    AREA_ID_NONE,
    AREA_ID_WHITE,
    AREA_ID_BLACK,
    AREA_ID_MOUNTAIN,
    AREA_ID_MOUNTAIN_RAINY,
    AREA_ID_CAVE,
    AREA_ID_CAVE_DEEP,
    AREA_ID_FOREST,
    AREA_ID_SAVE,
    //
    NUM_AREA_ID
};

typedef struct {
    i32 x;
} area_mountain_s;

typedef struct {
    i32 x;
} area_cave_s;

typedef struct area_s {
    i32             ID;
    //
    area_mountain_s mountain;
    area_cave_s     cave;
    //
    i32             fx_type;
    union {
        areafx_wind_s           wind;
        areafx_rain_s           rain;
        areafx_heat_s           heat;
        areafx_snow_s           snow;
        areafx_particles_calm_s particles_calm;
    } fx;
} area_s;

void area_setup(g_s *g, area_s *a, i32 ID, i32 areafx);
void area_update(g_s *g, area_s *a);
void area_draw_bg(g_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam);

#endif