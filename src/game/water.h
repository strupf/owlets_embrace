// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef WATER_H
#define WATER_H

#include "os/os.h"

typedef struct {
        i32 y_q12;
        i32 v_q12;
} waterparticle_s;

typedef struct {
        waterparticle_s *particles;
        int              nparticles;
        v2_i32           p;
        i32              fzero_q16;
        i32              fneighbour_q16;
        i32              dampening_q12;
        int              loops;
} watersurface_s;

typedef struct {
        watersurface_s *surfaces;
        int             nsurfaces;
} water_s;

typedef struct {
        watersurface_s water;
        watersurface_s ocean;
        int            y;
} ocean_s;

int    ocean_base_amplitude(ocean_s *o, int at);
int    ocean_amplitude(ocean_s *o, int at);
void   ocean_update(ocean_s *o);
void   water_update(watersurface_s *ws);
int    water_amplitude(watersurface_s *ws, int at);
void   water_impact(watersurface_s *ws, int x, int r, int peak);
bool32 water_overlaps_pt(water_s *w, v2_i32 p);

#endif