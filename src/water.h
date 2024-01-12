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
    i32              fzero_q16;
    i32              fneighbour_q16;
    i32              dampening_q12;
    int              loops;
} watersurface_s;

void water_update(watersurface_s *ws);
int  water_amplitude(watersurface_s *ws, int at);
void water_impact(watersurface_s *ws, int x, int r, int peak);

#endif