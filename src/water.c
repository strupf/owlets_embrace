// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "water.h"
#include "game.h"

#define NUM_WATER_TILES 128

i32 water_tile_get(i32 x, i32 y, i32 tick)
{
    return ((x + (tick >> 1)) & (NUM_WATER_TILES - 1));
}

typedef struct {
    i32 y_q12;
    i32 v_q12;
} waterparticle_s;

static void water_step(waterparticle_s *particles, int num, int steps);

void water_prerender_tiles()
{
    tex_s wtex = tex_create(32, NUM_WATER_TILES * 16, asset_allocator);
    tex_clr(wtex, GFX_COL_CLEAR);
    asset_tex_putID(TEXID_WATER_PRERENDER, wtex);

    spm_push();

#define WATER_RENDER_STEP   2
#define WATER_NUM_PARTICLES 256

    usize            psize     = sizeof(waterparticle_s) * WATER_NUM_PARTICLES;
    waterparticle_s *particles = (waterparticle_s *)spm_allocz(psize);

    water_step(particles, WATER_NUM_PARTICLES, 256);

    gfx_ctx_s wctx  = gfx_ctx_default(wtex);
    gfx_ctx_s wctx1 = gfx_ctx_default(wtex);
    gfx_ctx_s wctx2 = gfx_ctx_default(wtex);
    wctx1.pat       = gfx_pattern_2x2(B2(11),
                                      B2(10));
    wctx2.pat       = gfx_pattern_2x2(B2(10),
                                      B2(00));

    for (int n = 0; n < NUM_WATER_TILES; n++) {
        water_step(particles, WATER_NUM_PARTICLES, 1);
        for (int k = 0; k < 16; k += WATER_RENDER_STEP) {

            int hh = particles[((k + WATER_NUM_PARTICLES) >> 1)].y_q12 >> 12;
            hh     = clamp_i(hh, -4, +4);
            int yy = n * 16 + 4 + hh;

            // filled tile silhouette background
            rec_i32 rf = {k, yy - 1, WATER_RENDER_STEP, 12 + 1 - hh};
            gfx_rec_fill(wctx, rf, PRIM_MODE_BLACK);

            // tile overlay top
            rec_i32 rl1 = {k + 16, yy + 2, WATER_RENDER_STEP, 2};
            rec_i32 rl2 = {k + 16, yy + 4, WATER_RENDER_STEP, 1};
            rec_i32 rfb = {k + 16, yy, WATER_RENDER_STEP, 12 - hh};
            gfx_rec_fill(wctx1, rfb, PRIM_MODE_BLACK);
            gfx_rec_fill(wctx1, rl1, PRIM_MODE_WHITE);
            gfx_rec_fill(wctx2, rl2, PRIM_MODE_WHITE);
        }
    }

    spm_pop();
}

static void water_step(waterparticle_s *particles, int num, int steps)
{
    for (int step = 0; step < steps; step++) {
        // forces and velocity integration
        for (int i = 0; i < 3; i++) {
            for (int n = 0; n < num; n++) {
                waterparticle_s *pl = &particles[(n - 1 + num) % num];
                waterparticle_s *pr = &particles[(n + 1) % num];
                waterparticle_s *pc = &particles[n];

                i32 f0 = pl->y_q12 + pr->y_q12 - pc->y_q12 * 2;
                pc->v_q12 += (f0 * 100 - pc->y_q12 * 2000) >> 16;
            }

            for (int n = 0; n < num; n++) {
                particles[n].y_q12 += particles[n].v_q12;
            }
        }

        // dampening
        for (int n = 0; n < num; n++) {
            waterparticle_s *pc = &particles[n];

            pc->v_q12 = (pc->v_q12 * 4050 + (1 << 11)) >> 12;
            pc->v_q12 += rngr_sym_i32(128);
        }
    }
}
