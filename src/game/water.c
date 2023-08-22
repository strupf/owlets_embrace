// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "water.h"

void watersurface_update(watersurface_s *ws)
{
        // udpate spring forces
        for (int n = 1; n < ws->nparticles - 1; n++) {
                waterparticle_s *pl = &ws->particles[n - 1];
                waterparticle_s *pr = &ws->particles[n + 1];
                waterparticle_s *pc = &ws->particles[n];
        }

        // integrate velocities
        for (int n = 0; n < ws->nparticles; n++) {
                waterparticle_s *pc = &ws->particles[n];
        }
}

bool32 water_overlaps_pt(water_s *w, v2_i32 p)
{
        return 0;
}