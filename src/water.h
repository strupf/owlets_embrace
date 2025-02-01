// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef WATER_H
#define WATER_H

#include "gamedef.h"

#define NUM_WATER       16
#define OCEAN_W_WORDS   PLTF_DISPLAY_WWORDS
#define OCEAN_W_PX      PLTF_DISPLAY_W
#define OCEAN_H_PX      PLTF_DISPLAY_H
#define OCEAN_NUM_SPANS 512

typedef struct fluid_pt_s {
    ALIGNAS(4)
    i16 y_q8;
    i16 v_q8;
} fluid_pt_s;

enum {
    FLUID_AREA_WATER,
    FLUID_AREA_LAVA,
};

typedef struct {
    fluid_pt_s *pts;
    u16         n;
    u16         d_q16;
    i16         c1;
    i16         c2;
    u8          steps;
    u8          r;
    i8          min_y;
    i8          max_y;
} fluid_surface_s;

typedef struct {
    fluid_surface_s s;
    i32             type;
    i32             tick;
    i32             x;
    i32             y;
    i32             w;
    i32             h;
} fluid_area_s;

void          fluid_surface_step(fluid_surface_s *b);
void          fluid_area_update(fluid_area_s *b);
fluid_area_s *fluid_area_create(g_s *g, rec_i32 r, i32 type, b32 surface);
void          fluid_area_destroy(g_s *g, fluid_area_s *a);

// drawn in two passes:
// 0: background only
// ... objects get drawn
// 1: overlay and surface sprites
void fluid_area_draw(gfx_ctx_s ctx, fluid_area_s *b, v2_i32 cam, i32 pass);

enum {
    FLUID_AREA_IMPACT_COS,
    FLUID_AREA_IMPACT_FLAT,
};

void fluid_area_impact(fluid_area_s *b, i32 x_mid, i32 w, i32 str, i32 type);
i32  water_depth_rec(g_s *g, rec_i32 r);

#endif