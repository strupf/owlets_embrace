// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef VFX_AREA_H
#define VFX_AREA_H

#include "gamedef.h"

enum {
    VFX_ID_NONE,
    VFX_ID_SNOW,
    VFX_ID_HEAT,
};

#define VFX_AREA_SNOW_NUM_SNOWFLAKES 256
#define VFX_AREA_SNOW_W              512
#define VFX_AREA_SNOW_H              256

typedef struct vfx_area_snowflake_s {
    v2_i16 p_q4;
    v2_i16 v_q4;
} vfx_area_snowflake_s;

typedef struct vfx_area_snow_s {
    i32                  type;
    i32                  n;
    vfx_area_snowflake_s snowflakes[VFX_AREA_SNOW_NUM_SNOWFLAKES];
} vfx_area_snow_s;

void vfx_area_snow_setup(g_s *g);
void vfx_area_snow_update(g_s *g);
void vfx_area_snow_draw(g_s *g, v2_i32 cam);

typedef struct vfx_area_heat_s {
    i32 tick;
} vfx_area_heat_s;

void vfx_area_heat_setup(g_s *g);
void vfx_area_heat_update(g_s *g);
void vfx_area_heat_draw(g_s *g, v2_i32 cam);

#endif