// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BACKFOREGROUND_H
#define BACKFOREGROUND_H

#include "game_def.h"

enum {
        BACKGROUND_WIND_CIRCLE_R = 1900,
};

enum {
        BG_NUM_CLOUDS      = 64,
        BG_NUM_PARTICLES   = 512,
        BG_NUM_CLOUD_TYPES = 3,
        BG_WIND_PARTICLE_N = 8,
};

enum {

        BG_PLANTS_MAX_EXCURSION_Q8 = 256,
        BG_PLANTS_PRERENDER_SHIFT  = 4,
        BG_PLANTS_FRAMES           = 256 >> BG_PLANTS_PRERENDER_SHIFT,
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
} windparticle_s;

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
        int    x_q8;
        int    v_q8;
} grass_s;

typedef struct {
        v2_i32 p_q8;
        v2_i32 v_q8;
        v2_i32 a_q8;
        int    ticks;
        int    ticks_og;
        int    size;
} particle_s;

typedef struct {
        parallax_img_s parallax;

        int        n_particles;
        particle_s particles[NUM_PARTICLES];

        cloudbg_s      clouds[BG_NUM_CLOUDS];
        int            n_clouds;
        windparticle_s windparticles[BG_NUM_PARTICLES];
        int            n_windparticles;
        int            clouddirection;

        grass_s grass[256];
        int     n_grass;

} backforeground_s;

void backforeground_setup(game_s *g, backforeground_s *b);
void backforeground_animate(game_s *g, backforeground_s *b);

typedef struct {
        v2_i32 p_q8;
        v2_i32 p_q8_spread;
        v2_i32 v_q8;
        v2_i32 v_q8_spread;
        v2_i32 a_q8;
        v2_i32 a_q8_spread;
        int    tick;
        int    tick_spread;
        int    size;
        int    n;

} bgpartice_desc_s;

void backforeground_spawn_particles(backforeground_s *b, bgpartice_desc_s d);

#endif