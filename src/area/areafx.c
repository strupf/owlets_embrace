// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
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
    int roll          = rngr_i32(0, 100);
    int size          = 0;
    if (roll <= 70) {
        size = 0;
    } else if (roll <= 90) {
        size = 1;
    } else {
        size = 2;
    }
    switch (size) {
    case 0: {
        int t    = rngr_i32(0, 3);
        c->t.r.x = 3 * t * 32;
        c->t.r.w = 3 * 32;
        c->t.r.h = 3 * 16;
    } break;
    case 1: {
        int t    = rngr_i32(0, 1);
        t        = 0;
        c->t.r.y = 3 * 16;
        c->t.r.x = 4 * t * 32 * 4;
        c->t.r.w = 4 * 32;
        c->t.r.h = 4 * 16;
    } break;
    case 2: {
        int t    = rngr_i32(0, 1);
        t        = 0;
        c->t.r.y = 7 * 16;
        c->t.r.x = t * 32 * 5;
        c->t.r.w = 5 * 32;
        c->t.r.h = 16 * 5;
    } break;
    }

    c->vx_q8    = (1 << rngr_i32(0, 7)) * (rngr_i32(0, 1) * 2 - 1);
    c->p.x      = 0 < c->vx_q8 ? -100 : +800;
    c->p.y      = rngr_i32(-70, 200);
    c->priority = (rng_u32() & 0xFFFF);
    return c;
}

void areafx_clouds_setup(game_s *g, areafx_clouds_s *fx)
{
    fx->n = 0;
    int N = rngr_i32(8, 16);
    for (int n = 0; n < N; n++) {
        areafx_cloud_s *c = areafx_cloud_create(fx);
        if (!c) break;
        c->p.x = rngr_i32(-100, 600);
    }
}

// called at 25 FPS (half frequency)
void areafx_clouds_update(game_s *g, areafx_clouds_s *fx)
{
    for (int n = fx->n - 1; 0 <= n; n--) {
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

int areafx_cloud_cmp(const void *a, const void *b)
{
    const areafx_cloud_s *x = (const areafx_cloud_s *)a;
    const areafx_cloud_s *y = (const areafx_cloud_s *)b;
    return (x->priority - y->priority);
}

void areafx_clouds_draw(game_s *g, areafx_clouds_s *fx, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();

    if (fx->dirty) {
        fx->dirty = 0;
        sort_array(fx->clouds, fx->n, sizeof(areafx_cloud_s), areafx_cloud_cmp);
    }
    for (int n = 0; n < fx->n; n++) {
        areafx_cloud_s *c = &fx->clouds[n];
        v2_i32          p = {c->p.x + (cam.x * 1) / 16, c->p.y};
        p.x &= ~3;
        p.y &= ~1;
        gfx_spr(ctx, c->t, p, 0, 0);
    }
}

void areafx_rain_setup(game_s *g, areafx_rain_s *fx)
{
    fx->n_drops        = 0;
    fx->lightning_tick = 0;
    for (int n = 0; n < 256; n++) {
        areafx_rain_update(g, fx);
    }
}

void areafx_rain_update(game_s *g, areafx_rain_s *fx)
{
    if (fx->lightning_tick) {
        fx->lightning_tick--;
    }
    if (rngr_i32(0, 1000) <= 4) {
        fx->lightning_tick  = rngr_i32(6, 10);
        fx->lightning_twice = rngr_i32(0, 4) == 0;
    }

    for (int n = fx->n_drops - 1; 0 <= n; n--) {
        areafx_raindrop_s *drop = &fx->drops[n];

        drop->p.x += drop->v.x;
        drop->p.y += drop->v.y;

        if ((512 << 8) < drop->p.y) {
            *drop = fx->drops[--fx->n_drops];
        }
    }

    for (int k = 0; k < 4; k++) {
        if (AREAFX_RAIN_DROPS <= fx->n_drops) break;
        areafx_raindrop_s *drop = &fx->drops[fx->n_drops++];
        drop->p.x               = rngr_i32(0, 512) << 8;
        drop->p.y               = -(100 << 8);
        drop->v.y               = rngr_i32(2000, 2800);
        drop->v.x               = rngr_i32(100, 700);
    }
}

void areafx_rain_draw(game_s *g, areafx_rain_s *fx, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    if (fx->lightning_tick) {
        gfx_ctx_s ctx_light = ctx;

        if (5 < fx->lightning_tick) {
            ctx_light.pat = gfx_pattern_interpolate(1, 4);
        } else if (3 < fx->lightning_tick) {
            if (fx->lightning_twice) {
                ctx_light.pat = gfx_pattern_interpolate(0, 2);
            } else {
                ctx_light.pat = gfx_pattern_interpolate(1, 2);
            }
        } else {
            ctx_light.pat = gfx_pattern_interpolate(1, 8);
        }

        gfx_rec_fill(ctx_light, (rec_i32){0, 0, 400, 240}, PRIM_MODE_WHITE);
    }
    gfx_ctx_s ctxdrop = ctx;
    ctxdrop.pat       = gfx_pattern_4x4(B4(0000),
                                        B4(1111),
                                        B4(0000),
                                        B4(1111));
    for (int n = 0; n < fx->n_drops; n++) {
        areafx_raindrop_s *drop = &fx->drops[n];
        v2_i32             pos  = v2_shr(drop->p, 8);
        pos.x += cam.x;
        pos.x = ((pos.x & 511) + 512) & 511;

        rec_i32 rr1 = {pos.x, pos.y + 5, 3, 8};
        rec_i32 rr2 = {pos.x, pos.y, 2, 5};
        rec_i32 rr3 = {pos.x, pos.y + 1, 1, 6};
        gfx_rec_fill(ctx, rr1, PRIM_MODE_BLACK);
        gfx_rec_fill(ctx, rr2, PRIM_MODE_BLACK);
        gfx_rec_fill(ctxdrop, rr3, PRIM_MODE_WHITE);
    }
}

void areafx_wind_setup(game_s *g, areafx_wind_s *fx)
{
}

void areafx_wind_update(game_s *g, areafx_wind_s *fx)
{
    // UPDATE PARTICLES
    // traverse backwards to avoid weird removal while iterating
    for (int n = fx->n - 1; 0 <= n; n--) {
        areafx_windpt_s *p = &fx->p[n];
        if (p->p_q8.x < 0 || AREAFX_WIND_SIZEY < (p->p_q8.x >> 8)) {
            fx->p[n] = fx->p[--fx->n];
            continue;
        }

        p->circcooldown--;
        if (p->circcooldown <= 0 && rng_u32() < 0x1000000U) { // enter wind circle animation
            p->ticks        = rngr_u32(15, 20);
            p->circticks    = p->ticks;
            p->circc.x      = p->p_q8.x;
            p->circc.y      = p->p_q8.y - AREAFX_WIND_R;
            p->circcooldown = 60;
        }

        if (0 < p->circticks) { // run through circle but keep slowly moving forward
            i32 a     = (0x400 * (p->ticks - p->circticks)) / p->ticks;
            p->p_q8.x = p->circc.x + ((sin_q16(a) * AREAFX_WIND_R) >> 16);
            p->p_q8.y = p->circc.y + ((cos_q16(a) * AREAFX_WIND_R) >> 16);
            p->circc.x += 200;
            p->circticks--;
        } else {
            p->v_q8.y += rngr_sym_i32(60);
            p->v_q8.y = clamp_i(p->v_q8.y, -400, +400);
            p->p_q8   = v2_add(p->p_q8, p->v_q8);
        }

        // circular buffer of positions
        p->pos_q8[p->i] = p->p_q8;
        p->i            = (p->i + 1) & (AREAFX_WINDPT_N - 1);
    }

    // SPAWN PARTICLES
    if (fx->n < AREAFX_WINDPT && rng_u32() <= 0x10000000U) {
        areafx_windpt_s *p = &fx->p[fx->n++];
        *p                 = (areafx_windpt_s){0};
        p->p_q8.y          = rngr_i32(0, AREAFX_WIND_SIZEY << 8);
        p->v_q8.x          = rngr_i32(2000, 4000);
        p->circcooldown    = 10;
        for (int i = 0; i < AREAFX_WINDPT_N; i++)
            p->pos_q8[i] = p->p_q8;
    }
}

void areafx_wind_draw(game_s *g, areafx_wind_s *fx, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();

    for (int n = 0; n < fx->n; n++) {
        areafx_windpt_s p = fx->p[n];

        v2_i32    p1 = v2_shr(p.pos_q8[p.i], 8);
        const int y1 = p1.y;
        const int h1 = ((y1 + cam.y) % AREAFX_WIND_SIZEY + AREAFX_WIND_SIZEY) % AREAFX_WIND_SIZEY;
        p1.y         = h1;

        for (int i = 1; i < AREAFX_WINDPT_N; i++) {
            int    k  = (p.i + i) & (AREAFX_WINDPT_N - 1);
            v2_i32 p2 = v2_shr(p.pos_q8[k], 8);
            p2.y      = h1 + (p2.y - y1);

            gfx_lin_thick(ctx, p1, p2, PRIM_MODE_BLACK, 1);
            p1 = p2;
        }
    }
}

void areafx_heat_setup(game_s *g, areafx_heat_s *fx)
{
}

// called at 25 FPS (half frequency)
void areafx_heat_update(game_s *g, areafx_heat_s *fx)
{
    fx->tick += 2000;
    for (int i = 0; i < AREAFX_HEAT_ROWS; i++) {
        int s         = sin_q16(fx->tick + i * 3000);
        fx->offset[i] = (s * 3) >> 17;
    }
}

void areafx_heat_draw(game_s *g, areafx_heat_s *fx, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    tex_s     t   = ctx.dst;
    int       y2  = min_i(AREAFX_HEAT_ROWS, t.h);

    for (int y = 0; y < y2; y++) {
        int off = fx->offset[y];

        if (0 < off) {
            int a = +off;
            int b = 32 - off;
            for (int x = t.wword - 1; 0 < x; x--) {
                u32 *p  = &t.px[x + y * t.wword];
                u32  b1 = bswap32(*(p + 0));
                u32  b2 = bswap32(*(p - 1));
                *p      = bswap32((b1 >> a) | (b2 << b));
            }
        } else if (off < 0) {
            int a = -off;
            int b = 32 + off;
            for (int x = 0; x < t.wword - 1; x++) {
                u32 *p  = &t.px[x + y * t.wword];
                u32  b1 = bswap32(*(p + 0));
                u32  b2 = bswap32(*(p + 1));
                *p      = bswap32((b1 << a) | (b2 >> b));
            }
        }
    }
}

void areafx_leaves_setup(game_s *g, areafx_leaves_s *fx)
{
}

void areafx_leaves_update(game_s *g, areafx_leaves_s *fx)
{
}

void areafx_leaves_draw(game_s *g, areafx_leaves_s *fx, v2_i32 cam)
{
}

void areafx_particles_calm_setup(game_s *g, areafx_particles_calm_s *fx)
{
    for (int n = 0; n < AREAFX_PT_CALM_N; n++) {
        areafx_particle_calm_s *p = &fx->p[n];

        p->pos.x = rngr_i32(0, PT_CALM_X_RANGE << 8);
        p->pos.y = rngr_i32(0, PT_CALM_Y_RANGE << 8);
        p->vel.x = rngr_sym_i32(PT_CALM_VCAP);
        p->vel.y = rngr_sym_i32(PT_CALM_VCAP);
    }
}

void areafx_particles_calm_update(game_s *g, areafx_particles_calm_s *fx)
{
    for (int n = 0; n < AREAFX_PT_CALM_N; n++) {
        areafx_particle_calm_s *p = &fx->p[n];

        p->pos = v2_add(p->pos, p->vel);
        p->vel.x += rngr_sym_i32(PT_CALM_VRNG);
        p->vel.y += rngr_sym_i32(PT_CALM_VRNG);
        p->vel.x = clamp_i32(p->vel.x, -PT_CALM_VCAP, +PT_CALM_VCAP);
        p->vel.y = clamp_i32(p->vel.y, -PT_CALM_VCAP, +PT_CALM_VCAP);
    }
}

void areafx_particles_calm_draw(game_s *g, areafx_particles_calm_s *fx, v2_i32 cam)
{
    const gfx_ctx_s ctx = gfx_ctx_display();

    for (int n = 0; n < AREAFX_PT_CALM_N; n++) {
        areafx_particle_calm_s p = fx->p[n];

        v2_i32 pos = v2_add(v2_shr(p.pos, 8), cam);
        pos.x      = (pos.x & (PT_CALM_X_RANGE - 1)) - 16;
        pos.y      = (pos.y & (PT_CALM_Y_RANGE - 1)) - 16;
        gfx_cir_fill(ctx, pos, 4, PRIM_MODE_BLACK);
    }
}
