// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "areafx.h"
#include "game.h"

void areafx_particles_calm_setup(g_s *g, areafx_particles_calm_s *fx)
{
    for (i32 n = 0; n < AREAFX_PT_CALM_N; n++) {
        areafx_particle_calm_s *p = &fx->p[n];

        p->pos.x = rngr_i32(0, PT_CALM_X_RANGE << 8);
        p->pos.y = rngr_i32(0, PT_CALM_Y_RANGE << 8);
        p->vel.x = rngr_sym_i32(PT_CALM_VCAP);
        p->vel.y = rngr_sym_i32(PT_CALM_VCAP);
    }
}

void areafx_particles_calm_update(g_s *g, areafx_particles_calm_s *fx)
{
    for (i32 n = 0; n < AREAFX_PT_CALM_N; n++) {
        areafx_particle_calm_s *p = &fx->p[n];

        p->pos = v2_i32_add(p->pos, p->vel);
        p->vel.x += rngr_sym_i32(PT_CALM_VRNG);
        p->vel.y += rngr_sym_i32(PT_CALM_VRNG);
        p->vel.x = clamp_sym_i32(p->vel.x, PT_CALM_VCAP);
        p->vel.y = clamp_sym_i32(p->vel.y, PT_CALM_VCAP);
    }
}

void areafx_particles_calm_draw(g_s *g, areafx_particles_calm_s *fx, v2_i32 cam)
{
    const gfx_ctx_s ctx = gfx_ctx_display();

    for (i32 n = 0; n < AREAFX_PT_CALM_N; n++) {
        areafx_particle_calm_s p = fx->p[n];

        v2_i32 pos = v2_i32_add(v2_i32_shr(p.pos, 8), cam);
        pos.x      = (pos.x & (PT_CALM_X_RANGE - 1)) - 16;
        pos.y      = (pos.y & (PT_CALM_Y_RANGE - 1)) - 16;
        gfx_cir_fill(ctx, pos, 4, PRIM_MODE_BLACK);
    }
}
