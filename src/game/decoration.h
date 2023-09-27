// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef DECORATION_H
#define DECORATION_H

#include "game_def.h"

enum {
        BACKGROUND_WIND_CIRCLE_R = 1900,
};

enum {
        BG_NUM_CLOUDS      = 64,
        BG_NUM_PARTICLES   = 256,
        BG_NUM_CLOUD_TYPES = 3,
        BG_WIND_PARTICLE_N = 8,
};

typedef struct {
        int    cloudtype;
        v2_i32 p; // q8
        i32    v; // q8
} cloudbg_s;

typedef struct {
        int    n;
        v2_i32 p;
        v2_i32 v;
        v2_i32 pos[BG_WIND_PARTICLE_N];
        v2_i32 circc;
        i32    ticks;
        i32    circticks;
        i32    circcooldown;
} particlebg_s;

typedef struct {
        // values for Tiled's layer config
        f32 parallax_x;
        f32 parallax_y;
        f32 parallax_offx;
        f32 parallax_offy;
} parallax_img_s;

typedef struct {
        v2_i32 pos;
        int    type;
} grass_s;

typedef struct {
        v2_i32 p_q8;
        v2_i32 v_q8;
        v2_i32 a_q8;
        i32    ticks;
} particle_s;

typedef struct {
        parallax_img_s parallax;

        int        n_particles;
        particle_s particles[NUM_PARTICLES];

        cloudbg_s    clouds[BG_NUM_CLOUDS];
        int          nclouds;
        particlebg_s particlesbg[BG_NUM_PARTICLES];
        int          nparticles;
        int          clouddirection;

} backforeground_s;

void backforeground_setup(game_s *g, backforeground_s *b);
void backforeground_animate(game_s *g, backforeground_s *b);

void decoration_setup(game_s *g);
void decoration_animate(game_s *g);

#endif