// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "particle.h"
#include "game.h"

void particles_spawn(game_s *g, particles_s *pr, particle_desc_s desc, int n)
{
    int i2 = min_i(NUM_PARTICLES - pr->n, n);
    for (int i = 0; i < i2; i++) {
        v2_i32 pos = desc.p.p_q8;
        pos.x += rngr_i32(-desc.pr_q8.x, +desc.pr_q8.x);
        pos.y += rngr_i32(-desc.pr_q8.y, +desc.pr_q8.y);
        if (!game_traversable_pt(g, pos.x >> 8, pos.y >> 8)) continue;

        particle_s *p = &pr->particles[pr->n++];
        *p            = desc.p;
        p->p_q8       = pos;
        p->v_q8.x += rngr_i32(-desc.vr_q8.x, +desc.vr_q8.x);
        p->v_q8.y += rngr_i32(-desc.vr_q8.y, +desc.vr_q8.y);
        p->a_q8.x += rngr_i32(-desc.ar_q8.x, +desc.ar_q8.x);
        p->a_q8.y += rngr_i32(-desc.ar_q8.y, +desc.ar_q8.y);
        p->size += rngr_i32(0, desc.sizer);
        p->ticks_max += rngr_i32(0, desc.ticksr);
        p->ticks = p->ticks_max;
    }
}

void particles_update(game_s *g, particles_s *pr)
{
    for (int i = pr->n - 1; 0 <= i; i--) {
        particle_s *p  = &pr->particles[i];
        v2_i32      p0 = v2_shr(p->p_q8, 8);
        if (--p->ticks <= 0 || !game_traversable_pt(g, p0.x, p0.y)) {
            *p = pr->particles[--pr->n];
            continue;
        }

        p->v_q8   = v2_add(p->v_q8, p->a_q8);
        v2_i32 pp = v2_add(p->p_q8, p->v_q8);  // proposed new position
        v2_i32 pd = v2_sub(v2_shr(pp, 8), p0); // delta in pixels

        for (int m = abs_i(pd.x), s = sgn_i(pd.x); 0 < m; m--) {
            if (game_traversable_pt(g, p0.x + s, p0.y)) {
                p0.x += s;
            } else {
                p->v_q8.x = -(p->v_q8.x >> 1); // bounce off
                pp.x      = p0.x << 8;
                break;
            }
        }

        for (int m = abs_i(pd.y), s = sgn_i(pd.y); 0 < m; m--) {
            if (game_traversable_pt(g, p0.x, p0.y + s)) {
                p0.y += s;
            } else {
                p->v_q8.y = -(p->v_q8.y >> 1); // bounce off
                p->p_q8.y = p0.y << 8;
                pp.y      = p0.y << 8;
                break;
            }
        }

        p->p_q8 = pp;
    }
}
