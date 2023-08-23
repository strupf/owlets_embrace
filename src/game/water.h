// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef WATER_H
#define WATER_H

#include "gamedef.h"

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

void   watersurface_update(watersurface_s *ws);
int    water_amplitude(watersurface_s *ws, int at);
void   water_impact(watersurface_s *ws, int x, int r, int peak);
bool32 water_overlaps_pt(water_s *w, v2_i32 p);

#endif