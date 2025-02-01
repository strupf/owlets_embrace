// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "particle.h"
#include "game.h"

particle_emitter_s *particle_emitter_create(g_s *g)
{
    particle_sys_s *pr = &g->particle_sys;
    if (ARRLEN(pr->emitters) <= pr->n_emitters) return 0;

    particle_emitter_s *pe = &pr->emitters[pr->n_emitters++];
    mclr(pe, sizeof(particle_emitter_s));
    return pe;
}

void particle_emitter_destroy(g_s *g, particle_emitter_s *pe)
{
    particle_sys_s *pr = &g->particle_sys;

    *pe = pr->emitters[--pr->n_emitters];
}

void particle_emitter_emit(g_s *g, particle_emitter_s *e, i32 n)
{
    particle_sys_s *pr = &g->particle_sys;

    u32 s        = pltf_cur_tick();
    i32 to_spawn = min_i32(n, ARRLEN(pr->particles) - pr->n);
    mclr(&pr->particles[pr->n], to_spawn * sizeof(particle_s));

    for (i32 k = 0; k < to_spawn; k++) {
        particle_s *p = &pr->particles[pr->n++];
        p->p.x        = e->p.x + rngsr_sym_i32(&s, e->p_range.x);
        p->p.y        = e->p.y + rngsr_sym_i32(&s, e->p_range.y);
        p->v_q8.x     = e->v_q8.x + rngsr_sym_i32(&s, e->v_q8_range.x);
        p->v_q8.y     = e->v_q8.y + rngsr_sym_i32(&s, e->v_q8_range.y);
        p->a_q8.x     = e->a_q8.x + rngsr_sym_i32(&s, e->a_q8_range.x);
        p->a_q8.y     = e->a_q8.y + rngsr_sym_i32(&s, e->a_q8_range.y);
        p->ticks_max  = rngsr_u32(&s, e->ticks_min, e->ticks_max);
        p->mode       = e->mode;
        p->type       = e->type;

        switch (e->type & PARTICLE_MASK_TYPE) {
        case PARTICLE_TYPE_CIR:
        case PARTICLE_TYPE_REC: {
            p->prim.size_beg = rngsr_u32(&s, e->size_beg_min, e->size_beg_max);
            p->prim.size_end = rngsr_u32(&s, e->size_end_min, e->size_end_max);
            break;
        }
        case PARTICLE_TYPE_TEX: {
            p->tex = e->tex;
            break;
        }
        }
    }
}

void particle_sys_update(g_s *g)
{
    particle_sys_s *pr = &g->particle_sys;
#ifdef PLTF_DEV_ENV
    static i32 max_particles;
    if (max_particles < pr->n) {
        max_particles = pr->n;
        if (ARRLEN(pr->particles) / 2 < max_particles) {
            pltf_log("Particles in use: %i%%\n", (100 * max_particles) / ARRLEN(pr->particles));
        }
    }
#endif

    for (i32 i = pr->n_emitters - 1; 0 <= i; i--) {
        particle_emitter_s *pe = &pr->emitters[i];
        pe->tick++;
    }

    for (i32 i = pr->n - 1; 0 <= i; i--) {
        particle_s *p = &pr->particles[i];
        p->ticks++;

        if (p->ticks_max <= p->ticks ||
            ((p->type & PARTICLE_FLAG_COLLISIONS) &&
             map_blocked_pt(g, p->p.x, p->p.y))) {
            *p = pr->particles[--pr->n];
            continue;
        }

        p->p_q8 = v2_i16_add(p->p_q8, p->v_q8);
        p->v_q8 = v2_i16_add(p->v_q8, p->a_q8);
        i32 dx  = p->p_q8.x >> 8;
        i32 dy  = p->p_q8.y >> 8;
        p->p_q8.x &= 0xFF;
        p->p_q8.y &= 0xFF;
        if (p->type & PARTICLE_FLAG_COLLISIONS) {
            for (i32 m = abs_i32(dx), s = sgn_i32(dx); m; m--, p->p.x += s) {
                if (map_blocked_pt(g, p->p.x + s, p->p.y)) {
                    p->v_q8.x = -p->v_q8.x >> 1;
                    break;
                }
            }
            for (i32 m = abs_i32(dy), s = sgn_i32(dy); m; m--, p->p.y += s) {
                if (map_blocked_pt(g, p->p.x, p->p.y + s)) {
                    p->v_q8.y = -p->v_q8.y >> 1;
                    break;
                }
            }
        } else {
            p->p.x += dx;
            p->p.y += dy;
        }
    }
}

void particle_sys_draw(g_s *g, v2_i32 cam)
{
    particle_sys_s *pr  = &g->particle_sys;
    gfx_ctx_s       ctx = gfx_ctx_display();
    tex_s           tex = asset_tex(TEXID_PARTICLES);

    for (i32 i = 0; i < pr->n; i++) {
        particle_s *p    = &pr->particles[i];
        v2_i32      pos  = v2_i32_add(v2_i32_from_i16(p->p), cam);
        gfx_ctx_s   ctxp = ctx;
        if (p->type & PARTICLE_FLAG_FADE_OUT) {
            ctxp.pat = gfx_pattern_interpolate(p->ticks_max - p->ticks, p->ticks_max);
        }

        switch (p->type & PARTICLE_MASK_TYPE) {
        case PARTICLE_TYPE_CIR: {
            i32 s = lerp_i32(p->prim.size_beg, p->prim.size_end,
                             p->ticks, p->ticks_max);
            gfx_cir_fill(ctxp, pos, s, p->mode);
            break;
        }
        case PARTICLE_TYPE_REC: {
            i32     s = lerp_i32(p->prim.size_beg, p->prim.size_end,
                                 p->ticks, p->ticks_max);
            rec_i32 r = {pos.x - (s >> 1), pos.y - (s >> 1), s, s};
            gfx_rec_fill(ctxp, r, p->mode);
            break;
        }
        case PARTICLE_TYPE_TEX: {
            i32      f = lerp_i32(0, p->tex.n_frames, p->ticks, p->ticks_max);
            texrec_s t = {tex,
                          (p->tex.x + f * p->tex.w) << 3,
                          (p->tex.y) << 3,
                          (p->tex.w) << 3,
                          (p->tex.h) << 3};
            gfx_spr(ctxp, t, pos, p->mode, 0);
            break;
        }
        }
    }
}