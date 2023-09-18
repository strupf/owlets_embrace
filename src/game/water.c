// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "water.h"

static void i_water_update(watersurface_s *ws)
{
        const i32 FN_Q16 = ws->fneighbour_q16;
        const i32 FZ_Q16 = ws->fzero_q16;
        const int N      = ws->nparticles;

        // forces and velocity integration
        for (int i = 0; i < ws->loops; i++) {
                {
                        waterparticle_s *pr = &ws->particles[1];
                        waterparticle_s *pc = &ws->particles[0];

                        i32 f0 = pr->y_q12 - pc->y_q12 * 2;
                        pc->v_q12 += (f0 * FN_Q16 - pc->y_q12 * FZ_Q16) >> 16;
                }
                for (int n = 1; n < N - 1; n++) {
                        waterparticle_s *pl = &ws->particles[n - 1];
                        waterparticle_s *pr = &ws->particles[n + 1];
                        waterparticle_s *pc = &ws->particles[n];

                        i32 f0 = pl->y_q12 + pr->y_q12 - pc->y_q12 * 2;
                        pc->v_q12 += (f0 * FN_Q16 - pc->y_q12 * FZ_Q16) >> 16;
                }

                {
                        waterparticle_s *pl = &ws->particles[N - 2];
                        waterparticle_s *pc = &ws->particles[N - 1];

                        i32 f0 = pl->y_q12 - pc->y_q12 * 2;
                        pc->v_q12 += (f0 * FN_Q16 - pc->y_q12 * FZ_Q16) >> 16;
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

void water_impact_v(watersurface_s *ws, int x, int r, int peak)
{
        int i1 = (r >= x ? -x : -r);
        int i2 = (x + r >= ws->nparticles ? ws->nparticles - x : +r);

        for (int i = i1; i < i2; i++) {
                int a = (i * Q16_ANGLE_TURN) / (r << 1);
                int v = (((cos_q16_fast(a) * peak) >> 16) + peak) >> 1;

                ws->particles[x + i].v_q12 += v;
        }
}

void ocean_update(ocean_s *o)
{
        i_water_update(&o->ocean);
        if (rngf() < 0.5f) {
                int ii = rng_range(30, o->ocean.nparticles - 1);
                int rr = rng_range(5, 20);
                int pp = rng_range(-4000, +4000);
                water_impact_v(&o->ocean, ii, rr, pp);
        }

        water_update(&o->water);
}

int ocean_base_amplitude(ocean_s *o, int at)
{
        int o1 = at >> 2;
        int yy = o->y;
        if ((at & 3) == 0) {
                yy += water_amplitude(&o->ocean, o1);
        } else {
                int o2 = (at + 3) >> 2;

                yy += (water_amplitude(&o->ocean, o1) +
                       water_amplitude(&o->ocean, o2)) /
                      2;
        }
        return yy;
}

int ocean_amplitude(ocean_s *o, int at)
{
        return ocean_base_amplitude(o, at) + water_amplitude(&o->water, at);
}

void water_update(watersurface_s *ws)
{
        i_water_update(ws);
        for (int n = 0; n < ws->nparticles; n++) {
                waterparticle_s *pc = &ws->particles[n];
                pc->v_q12 += rng_range(-64, +64);
        }
}

void water_impact(watersurface_s *ws, int x, int r, int peak)
{
        int i1 = (r >= x ? -x : -r);
        int i2 = (x + r >= ws->nparticles ? ws->nparticles - x : +r);

        for (int i = i1; i < i2; i++) {
                int a = (i * Q16_ANGLE_TURN) / (r << 1);
                int v = (((cos_q16_fast(a) * peak) >> 16) + peak) >> 1;

                ws->particles[x + i].y_q12 = v;
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