// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "water.h"

void watersurface_update(watersurface_s *ws)
{
        const i32 FN_Q16 = ws->fneighbour_q16;
        const i32 FZ_Q16 = ws->fzero_q16;
        const int N      = ws->nparticles;

        // forces and velocity integration
        for (int i = 0; i < ws->loops; i++) {
                for (int n = 1; n < N - 1; n++) {
                        waterparticle_s *pl = &ws->particles[n - 1];
                        waterparticle_s *pr = &ws->particles[n + 1];
                        waterparticle_s *pc = &ws->particles[n];

                        i32 f0 = pl->y_q12 + pr->y_q12 - pc->y_q12 * 2;
                        i32 f  = f0 * FN_Q16 - pc->y_q12 * FZ_Q16;
                        pc->v_q12 += (f >> 16) + rng_range(-64, 64);
                }

                for (int n = 0; n < N; n++) {
                        ws->particles[n].y_q12 += ws->particles[n].v_q12;
                }
        }

        // dampening
        for (int n = 0; n < N; n++) {
                waterparticle_s *pc = &ws->particles[n];

                pc->v_q12 = (pc->v_q12 * ws->dampening_q12 + (1 << 11)) >> 12;
        }
}

void water_impact(watersurface_s *ws, int x, int r, int peak)
{
        for (int i = -r; i <= r; i++) {
                int c = x + i;
                if (0 > c || c >= ws->nparticles) continue;
                int a = (i * Q16_ANGLE_TURN) / (r << 1);
                int v = (((cos_q16_fast(a) * peak) >> 16) + peak) >> 1;

                ws->particles[c].y_q12 = v;
        }
}

int water_amplitude(watersurface_s *ws, int at)
{
        return ((ws->particles[at].y_q12 + (1 << 11)) >> 12) + ws->p.y;
}

bool32 water_overlaps_pt(water_s *w, v2_i32 p)
{
        return 0;
}