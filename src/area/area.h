// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _AREA_H
#define _AREA_H

#include "areafx.h"
#include "core/gfx.h"
#include "gamedef.h"

typedef struct {
    i32 x;
} area_mountain_s;

typedef struct {
    i32 x;
} area_cave_s;

typedef struct {
    i32             ID;
    //
    area_mountain_s mountain;
    area_cave_s     cave;
    //
    struct {
        areafx_clouds_s         clouds;
        areafx_wind_s           wind;
        areafx_rain_s           rain;
        areafx_heat_s           heat;
        areafx_leaves_s         leaves;
        areafx_snow_s           snow;
        areafx_particles_calm_s particles_calm;
    } fx;
} area_s;

void area_setup(g_s *g, area_s *a, i32 ID);
void area_update(g_s *g, area_s *a);
void area_draw_bg(g_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam);
void area_draw_mg(g_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam);
void area_draw_fg(g_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam);

#endif