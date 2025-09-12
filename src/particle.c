// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "particle.h"
#include "game.h"

void particle_sys_shuffle(particle_sys_s *pr, i32 n1, i32 n)
{
    for (i32 k = n - 1; 0 < k; k--) {
        i32 i = n1 + k;
        i32 j = n1 + rngr_i32(k, n - 1);
        SWAP(particle_s, pr->particles[i], pr->particles[j]);
    }
}

particle_emit_s *particle_emitter_create(g_s *g)
{
    particle_sys_s *pr = &g->particle_sys;

    for (i32 i = 0; i < ARRLEN(pr->emitters); i++) {
        particle_emit_s *pe = &pr->emitters[i];

        if (!pe->in_use) {
            mclr(pe, sizeof(particle_emit_s));
            pe->in_use = 1;
            return pe;
        }
    }
    return 0;
}

void particle_emitter_destroy(g_s *g, particle_emit_s *pe)
{
    pe->in_use = 0;
}

void particle_emitter_emit(g_s *g, particle_emit_s *e, i32 n)
{
    particle_sys_s *pr = &g->particle_sys;
    if (pr->seed == 0) {
        pr->seed = 213;
    }

    i32 to_spawn = min_i32(n, ARRLEN(pr->particles) - pr->n);
    mclr(&pr->particles[pr->n], to_spawn * sizeof(particle_s));

    for (i32 k = 0; k < to_spawn; k++) {
        particle_s *p = &pr->particles[pr->n++];

        p->p.x       = e->p.x + rngsr_sym_i32(&pr->seed, e->p_range.x);
        p->p.y       = e->p.y + rngsr_sym_i32(&pr->seed, e->p_range.y);
        p->v_q8.x    = e->v_q8.x + rngsr_sym_i32(&pr->seed, e->v_q8_range.x);
        p->v_q8.y    = e->v_q8.y + rngsr_sym_i32(&pr->seed, e->v_q8_range.y);
        p->a_q8.x    = e->a_q8.x + rngsr_sym_i32(&pr->seed, e->a_q8_range.x);
        p->a_q8.y    = e->a_q8.y + rngsr_sym_i32(&pr->seed, e->a_q8_range.y);
        p->ticks_max = rngsr_i32(&pr->seed, e->ticks_min, e->ticks_max);
        p->mode      = e->mode;
        p->type      = e->type;
        p->drag      = e->drag;

        obj_s *o = obj_from_handle(e->o);
        if (o) {
            p->p.x += o->pos.x;
            p->p.y += o->pos.y;
        }
        if (e->p_range_r) {
            i32 a = (i32)rngs_i32_bound(&pr->seed, 1 << 17);
            i32 r = (i32)rngs_i32_bound(&pr->seed, e->p_range_r);
            p->p.x += (sin_q15(a) * r) >> 15;
            p->p.y += (cos_q15(a) * r) >> 15;
        }

        switch (e->type & PARTICLE_MASK_TYPE) {
        case PARTICLE_TYPE_CIR:
        case PARTICLE_TYPE_REC: {
            p->prim.size_beg = rngsr_i32(&pr->seed, e->size_beg_min, e->size_beg_max);
            p->prim.size_end = rngsr_i32(&pr->seed, e->size_end_min, e->size_end_max);
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

    for (i32 i = 0; i < ARRLEN(pr->emitters); i++) {
        particle_emit_s *pe = &pr->emitters[i];
        if (!pe->in_use) continue;

        if (pe->o.o && !obj_handle_valid(pe->o)) {
            pe->in_use = 0;
            continue;
        }

        pe->tick++;
        if (pe->interval <= pe->tick) {
            pe->tick = 0;
            particle_emitter_emit(g, pe, pe->amount);
        }
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
        i32 dr    = 256 - p->drag;
        p->v_q8.x = (p->v_q8.x * dr) >> 8;
        p->v_q8.y = (p->v_q8.y * dr) >> 8;

#if 0
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
#else
        p->p.x += dx;
        p->p.y += dy;
#endif
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