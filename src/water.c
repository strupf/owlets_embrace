// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "water.h"

static void i_water_update(watersurface_s *ws)
{
    const i32 FN_Q16 = ws->fneighbour_q16;
    const i32 FZ_Q16 = ws->fzero_q16;
    const int L      = ws->nparticles - 1;

    // forces and velocity integration
    for (int i = 0; i < ws->loops; i++) {
        waterparticle_s *p = &ws->particles[0];
        for (int n = 0; n <= L; n++) {
            i32 f0 = p->y_q12 << 1;
            if (0 < n) f0 -= (p - 1)->y_q12;
            if (n < L) f0 -= (p + 1)->y_q12;
            p->v_q12 -= (p->y_q12 * FZ_Q16 + f0 * FN_Q16) >> 16;
            p->v_q12 = (p->v_q12 * ws->dampening_q12) >> 12;
            p++;
        }
        for (int n = 0; n <= L; n++) {
            p--;
            p->y_q12 += p->v_q12;
        }
    }
}

void water_update(watersurface_s *ws)
{
    i_water_update(ws);
    for (int n = 0; n < ws->nparticles; n++) {
        waterparticle_s *pc = &ws->particles[n];
        pc->v_q12 += rngr_sym_i32(64);
    }
}

void water_impact(watersurface_s *ws, int x, int r, int peak)
{
    int i1 = (r >= x ? -x : -r);
    int i2 = (x + r >= ws->nparticles ? ws->nparticles - x : +r);

    for (int i = i1; i < i2; i++) {
        int a = (i * Q16_ANGLE_TURN) / (r << 1);
        int v = (((cos_q16(a) * peak) >> 16) + peak) >> 1;

        ws->particles[x + i].y_q12 = v;
    }
}

int water_amplitude(watersurface_s *ws, int at)
{
    assert(0 <= at && at < ws->nparticles);
    return ((ws->particles[at].y_q12 + (1 << 11)) >> 12);
}