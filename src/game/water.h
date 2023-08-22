// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef WATER_H
#define WATER_H

#include "gamedef.h"

typedef struct {
        int x;
} waterparticle_s;

typedef struct {
        waterparticle_s *particles;
        int              nparticles;
        int              y;
} watersurface_s;

typedef struct {
        watersurface_s *surfaces;
        int             nsurfaces;
} water_s;

void   watersurface_update(watersurface_s *ws);
bool32 water_overlaps_pt(water_s *w, v2_i32 p);

#endif