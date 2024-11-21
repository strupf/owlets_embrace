// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "particle.h"
#include "game.h"

void particles_spawn(g_s *g, particle_desc_s desc, i32 n)
{
    particles_s *pr = &g->particles;
    if (pr->n == PARTICLE_NUM) return;
    for (i32 i = 0; i < n; i++) {
        v2_i32 pos = desc.p.p_q8;
        pos.x += rngr_sym_i32(desc.pr_q8.x);
        pos.y += rngr_sym_i32(desc.pr_q8.y);
        if (map_traversable_pt(g, pos.x >> 8, pos.y >> 8)) {
            particle_s *p = &pr->particles[pr->n++];
            *p            = desc.p;
            p->p_q8       = pos;
            p->v_q8.x += rngr_sym_i32(desc.vr_q8.x);
            p->v_q8.y += rngr_sym_i32(desc.vr_q8.y);
            p->a_q8.x += rngr_sym_i32(desc.ar_q8.x);
            p->a_q8.y += rngr_sym_i32(desc.ar_q8.y);
            p->size += rngr_i32(0, desc.sizer);
            p->ticks_max += rngr_i32(0, desc.ticksr);
            p->ticks = p->ticks_max;
            if (pr->n == PARTICLE_NUM) return;
        }
    }
}

void particles_update(g_s *g, particles_s *pr)
{
    for (i32 i = pr->n - 1; 0 <= i; i--) {
        particle_s *p  = &pr->particles[i];
        v2_i32      p0 = v2_shr(p->p_q8, 8);
        if (--p->ticks <= 0 || !map_traversable_pt(g, p0.x, p0.y)) {
            *p = pr->particles[--pr->n];
            continue;
        }

        p->v_q8   = v2_add(p->v_q8, p->a_q8);
        v2_i32 pp = v2_add(p->p_q8, p->v_q8);  // proposed new position
        v2_i32 pd = v2_sub(v2_shr(pp, 8), p0); // delta in pixels

        for (i32 m = abs_i32(pd.x), s = sgn_i32(pd.x); m; m--) {
            if (map_traversable_pt(g, p0.x + s, p0.y)) {
                p0.x += s;
            } else {
                p->v_q8.x = -(p->v_q8.x >> 1); // bounce off
                pp.x      = p0.x << 8;
                break;
            }
        }

        for (i32 m = abs_i32(pd.y), s = sgn_i32(pd.y); m; m--) {
            if (map_traversable_pt(g, p0.x, p0.y + s)) {
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

void particles_draw(g_s *g, particles_s *pr, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    for (i32 i = 0; i < pr->n; i++) {
        particle_s *p           = &pr->particles[i];
        v2_i32      ppos        = v2_add(v2_shr(p->p_q8, 8), cam);
        gfx_ctx_s   ctxparticle = ctx;
        ctxparticle.pat         = gfx_pattern_interpolate(p->ticks, p->ticks_max);

        switch (p->gfx) {
        case PARTICLE_GFX_CIR: {
            gfx_cir_fill(ctxparticle, ppos, p->size, p->col);
        } break;
        case PARTICLE_GFX_REC: {
            rec_i32 rr = {ppos.x, ppos.y, p->size, p->size};
            gfx_rec_fill(ctxparticle, rr, p->col);
        } break;
        case PARTICLE_GFX_SPR: {
            gfx_spr(ctxparticle, p->texrec, ppos, 0, 0);
        } break;
        }
    }
}