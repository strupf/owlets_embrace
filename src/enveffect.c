// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "enveffect.h"
#include "game.h"

static cloud_s *cloud_create(enveffect_cloud_s *e)
{
    if (e->n == BG_NUM_CLOUDS) return NULL;
    cloud_s *c = &e->clouds[e->n++];
    *c         = (cloud_s){0};
    e->dirty   = 1;
    c->t       = asset_texrec(TEXID_CLOUDS, 0, 0, 0, 0);
    int roll   = rngr_i32(0, 100);
    int size   = 0;
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
    c->p.x      = 0 < c->vx_q8 ? -200 : +600;
    c->p.y      = rngr_i32(-70, 180);
    c->priority = (rng_u32() & 0xFFFF);
    return c;
}

void enveffect_cloud_setup(enveffect_cloud_s *e)
{
    e->n  = 0;
    int N = rngr_i32(8, 16);
    for (int n = 0; n < N; n++) {
        cloud_s *c = cloud_create(e);
        if (!c) break;
        c->p.x = rngr_i32(0, 400);
    }
}

// called at 25 FPS (half frequency)
void enveffect_cloud_update(enveffect_cloud_s *e)
{
    for (int n = e->n - 1; 0 <= n; n--) {
        cloud_s *c = &e->clouds[n];

        if ((c->vx_q8 < 0 && c->p.x < -200) &&
            (c->vx_q8 > 0 && c->p.x > +500)) {
            *c       = e->clouds[--e->n];
            e->dirty = 1;
            continue;
        }

        c->mx_q8 += c->vx_q8;
        c->p.x += (c->mx_q8 >> 8);
        c->mx_q8 &= 0xFF;
    }

    if (rng_u32() < 0x10000000U) {
        cloud_s *c = cloud_create(e);
    }
}

int cmp_cloud(const void *a, const void *b)
{
    const cloud_s *x = (const cloud_s *)a;
    const cloud_s *y = (const cloud_s *)b;
    return (x->priority - y->priority);
}

void enveffect_cloud_draw(gfx_ctx_s ctx, enveffect_cloud_s *e, v2_i32 cam)
{
    if (e->dirty) {
        e->dirty = 0;
        sort_array(e->clouds, e->n, sizeof(cloud_s), cmp_cloud);
    }
    for (int n = 0; n < e->n; n++) {
        cloud_s *c = &e->clouds[n];
        v2_i32   p = c->p;
        p.x &= ~3;
        p.y &= ~1;
        gfx_spr(ctx, c->t, p, 0, 0);
    }
}

void enveffect_wind_update(enveffect_wind_s *e)
{
    // UPDATE PARTICLES
    // traverse backwards to avoid weird removal while iterating
    for (int n = e->n - 1; 0 <= n; n--) {
        windparticle_s *p = &e->p[n];
        if (p->p_q8.x < 0 || BG_SIZE < (p->p_q8.x >> 8)) {
            e->p[n] = e->p[--e->n];
            continue;
        }

        p->circcooldown--;
        if (p->circcooldown <= 0 && rng_u32() < 0x1000000U) { // enter wind circle animation
            p->ticks        = rngr_u32(15, 20);
            p->circticks    = p->ticks;
            p->circc.x      = p->p_q8.x;
            p->circc.y      = p->p_q8.y - BG_WIND_CIRCLE_R;
            p->circcooldown = 60;
        }

        if (0 < p->circticks) { // run through circle but keep slowly moving forward
            i32 a     = (0x400 * (p->ticks - p->circticks)) / p->ticks;
            p->p_q8.x = p->circc.x + ((sin_q16(a) * BG_WIND_CIRCLE_R) >> 16);
            p->p_q8.y = p->circc.y + ((cos_q16(a) * BG_WIND_CIRCLE_R) >> 16);
            p->circc.x += 200;
            p->circticks--;
        } else {
            p->v_q8.y += rngr_sym_i32(60);
            p->v_q8.y = clamp_i(p->v_q8.y, -400, +400);
            p->p_q8   = v2_add(p->p_q8, p->v_q8);
        }

        // circular buffer of positions
        p->pos_q8[p->i] = p->p_q8;
        p->i            = (p->i + 1) & (BG_WIND_PARTICLE_N - 1);
    }

    // SPAWN PARTICLES
    if (e->n < BG_NUM_PARTICLES && rng_u32() <= 0x10000000U) {
        windparticle_s *p = &e->p[e->n++];
        *p                = (windparticle_s){0};
        p->p_q8.y         = rngr_i32(0, BG_SIZE << 8);
        p->v_q8.x         = rngr_i32(2000, 4000);
        p->circcooldown   = 10;
        for (int i = 0; i < BG_WIND_PARTICLE_N; i++)
            p->pos_q8[i] = p->p_q8;
    }
}

void enveffect_wind_draw(gfx_ctx_s ctx, enveffect_wind_s *e, v2_i32 cam)
{
    for (int n = 0; n < e->n; n++) {
        windparticle_s p = e->p[n];

        v2_i32    p1 = v2_shr(p.pos_q8[p.i], 8);
        const int y1 = p1.y;
        const int h1 = ((y1 + cam.y) % BG_SIZE + BG_SIZE) % BG_SIZE;
        p1.y         = h1;

        for (int i = 1; i < BG_WIND_PARTICLE_N; i++) {
            int    k  = (p.i + i) & (BG_WIND_PARTICLE_N - 1);
            v2_i32 p2 = v2_shr(p.pos_q8[k], 8);
            p2.y      = h1 + (p2.y - y1);

            gfx_lin_thick(ctx, p1, p2, PRIM_MODE_BLACK, 1);
            p1 = p2;
        }
    }
}

// called at 25 FPS (half frequency)
void enveffect_heat_update(enveffect_heat_s *e)
{
    e->tick += 2000;
    for (int i = 0; i < BG_NUM_HEAT_ROWS; i++) {
        int s        = sin_q16(e->tick + i * 3000);
        e->offset[i] = (s * 2 - 1) >> 16;
    }
}

void enveffect_heat_draw(gfx_ctx_s ctx, enveffect_heat_s *e, v2_i32 cam)
{
    tex_s dst = ctx.dst;
    int   y2  = min_i(BG_NUM_HEAT_ROWS, dst.h);
    for (int y = 0; y < y2; y++) {
        int off = e->offset[y];
        if (0 < off) {
            int a = off;
            int b = 32 - off;
            for (int x = dst.wword - 1; 0 < x; x--) {
                int i              = x + y * dst.wword;
                u32 b1             = bswap32(((u32 *)dst.px)[i]);
                u32 b2             = bswap32(((u32 *)dst.px)[i - 1]);
                ((u32 *)dst.px)[i] = bswap32((b1 >> a) | (b2 << b));
            }
        } else if (off < 0) {
            int a = -off;
            int b = 32 + off;
            for (int x = 0; x < dst.wword - 1; x++) {
                int i              = x + y * dst.wword;
                u32 b1             = bswap32(((u32 *)dst.px)[i]);
                u32 b2             = bswap32(((u32 *)dst.px)[i + 1]);
                ((u32 *)dst.px)[i] = bswap32((b1 << a) | (b2 >> b));
            }
        }
    }
}

void backforeground_animate_grass(game_s *g)
{
    for (int n = 0; n < g->n_grass; n++) {
        grass_s *gr = &g->grass[n];
        rec_i32  r  = {gr->pos.x, gr->pos.y, 16, 16};

        for (obj_each(g, o)) {
            if ((o->flags & OBJ_FLAG_MOVER) && overlap_rec(r, obj_aabb(o))) {
                gr->v_q8 += o->vel_q8.x >> 4;
            }
        }

        gr->v_q8 += rngr_sym_i32(6) - ((gr->x_q8 * 15) >> 8);
        gr->x_q8 += gr->v_q8;
        gr->x_q8 = clamp_i(gr->x_q8, -256, +256);
        gr->v_q8 = (gr->v_q8 * 230) >> 8;
    }
}