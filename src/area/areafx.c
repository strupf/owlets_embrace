// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "areafx.h"
#include "game.h"

static areafx_cloud_s *areafx_cloud_create(areafx_clouds_s *fx)
{
    if (fx->n == AREAFX_CLOUDS) return NULL;
    areafx_cloud_s *c = &fx->clouds[fx->n++];
    *c                = (areafx_cloud_s){0};
    fx->dirty         = 1;
    c->t              = asset_texrec(TEXID_CLOUDS, 0, 0, 0, 0);
    i32 roll          = rngr_i32(0, 100);
    i32 size          = 0;
    if (roll <= 70) {
        size = 0;
    } else if (roll <= 90) {
        size = 1;
    } else {
        size = 2;
    }
    switch (size) {
    case 0: {
        i32 t  = rngr_i32(0, 3);
        c->t.x = 3 * t * 32;
        c->t.w = 3 * 32;
        c->t.h = 3 * 16;
    } break;
    case 1: {
        i32 t  = rngr_i32(0, 1);
        t      = 0;
        c->t.y = 3 * 16;
        c->t.x = 4 * t * 32 * 4;
        c->t.w = 4 * 32;
        c->t.h = 4 * 16;
    } break;
    case 2: {
        i32 t  = rngr_i32(0, 1);
        t      = 0;
        c->t.y = 7 * 16;
        c->t.x = t * 32 * 5;
        c->t.w = 5 * 32;
        c->t.h = 16 * 5;
    } break;
    }

    c->vx_q8    = (1 << rngr_i32(0, 7)) * (rngr_i32(0, 1) * 2 - 1);
    c->p.x      = 0 < c->vx_q8 ? -100 : +800;
    c->p.y      = rngr_i32(-70, 200);
    c->priority = (rng_u32() & 0xFFFF);
    return c;
}

void areafx_clouds_setup(g_s *g, areafx_clouds_s *fx)
{
    fx->n = 0;
    i32 N = rngr_i32(8, 16);
    for (i32 n = 0; n < N; n++) {
        areafx_cloud_s *c = areafx_cloud_create(fx);
        if (!c) break;
        c->p.x = rngr_i32(-100, 600);
    }
}

// called at 25 FPS (half frequency)
void areafx_clouds_update(g_s *g, areafx_clouds_s *fx)
{
    for (i32 n = fx->n - 1; 0 <= n; n--) {
        areafx_cloud_s *c = &fx->clouds[n];

        if ((c->vx_q8 < 0 && c->p.x < -200) &&
            (c->vx_q8 > 0 && c->p.x > +700)) {
            *c        = fx->clouds[--fx->n];
            fx->dirty = 1;
            continue;
        }

        c->mx_q8 += c->vx_q8;
        c->p.x += (c->mx_q8 >> 8);
        c->mx_q8 &= 0xFF;
    }

    if (rng_u32() < 0x10000000U) {
        areafx_cloud_s *c = areafx_cloud_create(fx);
    }
}

i32 areafx_cloud_cmp(const void *a, const void *b)
{
    const areafx_cloud_s *x = (const areafx_cloud_s *)a;
    const areafx_cloud_s *y = (const areafx_cloud_s *)b;
    return (x->priority - y->priority);
}

void areafx_clouds_draw(g_s *g, areafx_clouds_s *fx, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();

    if (fx->dirty) {
        fx->dirty = 0;
        sort_array(fx->clouds, fx->n, sizeof(areafx_cloud_s), areafx_cloud_cmp);
    }
    for (i32 n = 0; n < fx->n; n++) {
        areafx_cloud_s *c = &fx->clouds[n];
        v2_i32          p = {c->p.x + (cam.x * 1) / 16, c->p.y};
        p.x &= ~3;
        p.y &= ~1;
        gfx_spr(ctx, c->t, p, 0, 0);
    }
}

void areafx_leaves_setup(g_s *g, areafx_leaves_s *fx)
{
}

void areafx_leaves_update(g_s *g, areafx_leaves_s *fx)
{
}

void areafx_leaves_draw(g_s *g, areafx_leaves_s *fx, v2_i32 cam)
{
}

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

        p->pos = v2_add(p->pos, p->vel);
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

        v2_i32 pos = v2_add(v2_shr(p.pos, 8), cam);
        pos.x      = (pos.x & (PT_CALM_X_RANGE - 1)) - 16;
        pos.y      = (pos.y & (PT_CALM_Y_RANGE - 1)) - 16;
        gfx_cir_fill(ctx, pos, 4, PRIM_MODE_BLACK);
    }
}
