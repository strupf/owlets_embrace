// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef _AREA_H
#define _AREA_H

#include "areafx.h"
#include "core/gfx.h"
#include "gamedef.h"

typedef struct {
    int x;
} area_mountain_s;

typedef struct {
    int x;
} area_cave_s;

typedef struct {
    int             ID;
    int             tick;
    //
    area_mountain_s mountain;
    area_cave_s     cave;
    //
    struct {
        areafx_clouds_s clouds;
        areafx_wind_s   wind;
        areafx_rain_s   rain;
        areafx_heat_s   heat;
        areafx_leaves_s leaves;
    } fx;
} area_s;

void area_setup(game_s *g, area_s *a, int ID);
void area_update(game_s *g, area_s *a);
void area_draw_bg(game_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam);
void area_draw_mg(game_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam);
void area_draw_fg(game_s *g, area_s *a, v2_i32 cam_al, v2_i32 cam);

#endif