// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "decoration.h"
#include "game.h"

// called at half update rate!
static void       tileanimations_update();
static cloudbg_s *spawn_cloud(game_s *g);
static void       decoration_clouds(game_s *g);
static void       decoration_wind_particles(game_s *g);

void decoration_setup(game_s *g)
{
        for (int n = 0; n < 16; n++) {
                cloudbg_s *c = spawn_cloud(g);
                c->p.x       = rng_range(-100, 400 + 100) << 8;
                c->p.y       = rng_range(-100, 240) << 8;
        }
}

void room_deco_animate(game_s *g)
{
        tileanimations_update();
        decoration_clouds(g);
        decoration_wind_particles(g);
        for (int n = g->n_particles - 1; n >= 0; n--) {
                particle_s *p = &g->particles[n];
                if (--p->ticks == 0) {
                        g->particles[n] = g->particles[--g->n_particles];
                        continue;
                }
                p->v_q8 = v2_add(p->v_q8, p->a_q8);
                p->p_q8 = v2_add(p->p_q8, p->v_q8);
        }
}

static cloudbg_s *spawn_cloud(game_s *g)
{
        ASSERT(g->nclouds < BG_NUM_CLOUDS);
        cloudbg_s *c = &g->clouds[g->nclouds++];
        c->cloudtype = rng_max_u32(BG_NUM_CLOUD_TYPES - 1);
        c->v         = g->clouddirection * rng_range(4, 16);
        c->p.y       = rng_range(-50, 240) << 8;

        if (c->v < 0) {
                c->p.x = (400 + 100) << 8;
        } else {
                c->p.x = -(100 << 8);
        }
        return c;
}

static void decoration_clouds(game_s *g)
{
        for (int n = g->nclouds - 1; n >= 0; n--) {
                cloudbg_s *c = &g->clouds[n];
                c->p.x += c->v;

                if ((c->v > 0 && c->p.x > (g->pixel_x << 8)) ||
                    (c->v < 0 && c->p.x < 0)) {
                        g->clouds[n] = g->clouds[--g->nclouds];
                }
        }

        if (g->nclouds < BG_NUM_CLOUDS && rng_fast_u16() <= 200) {
                cloudbg_s *c = spawn_cloud(g);
        }
}

static void decoration_wind_particles(game_s *g)
{
        // traverse backwards to avoid weird removal while iterating
        for (int n = g->nparticles - 1; n >= 0; n--) {
                particlebg_s *p = &g->particlesbg[n];
                p->circcooldown--;
                if (p->circcooldown <= 0 && rng_fast_u16() < 600) { // enter wind circle animation
                        p->ticks        = rng_range_u32(15, 20);
                        p->circticks    = p->ticks;
                        p->circc.x      = p->p.x;
                        p->circc.y      = p->p.y - BACKGROUND_WIND_CIRCLE_R;
                        p->circcooldown = p->circticks + 70;
                }

                if (p->circticks > 0) { // run through circle but keep slowly moving forward
                        i32 a  = (Q16_ANGLE_TURN * (p->ticks - p->circticks)) / p->ticks;
                        int xx = sin_q16_fast(a) * BACKGROUND_WIND_CIRCLE_R;
                        int yy = cos_q16_fast(a) * BACKGROUND_WIND_CIRCLE_R;
                        p->p.x = p->circc.x + (xx >> 16);
                        p->p.y = p->circc.y + (yy >> 16);
                        p->circc.x += 200;
                        p->circticks--;
                } else {
                        p->p = v2_add(p->p, p->v);
                        p->v.y += rng_range(-60, +60);
                        p->v.y = CLAMP(p->v.y, -400, +400);
                }

                p->pos[p->n] = p->p;
                p->n         = (p->n + 1) & (BG_WIND_PARTICLE_N - 1);

                if ((p->p.x >> 8) < -200 || (p->p.x >> 8) > g->pixel_x + 200) {
                        g->particlesbg[n] = g->particlesbg[--g->nparticles];
                }
        }

        if (g->nparticles < BG_NUM_PARTICLES && rng_fast_u16() <= 2000) {
                particlebg_s *p = &g->particlesbg[g->nparticles++];
                p->p.x          = -(10 << 8);
                p->p.y          = rng_range(0, g->pixel_y) << 8;
                p->v.x          = rng_range(2000, 4000);
                p->v.y          = 0;
                p->circcooldown = 10;
                p->circticks    = 0;
                p->n            = 0;
                for (int i = 0; i < BG_WIND_PARTICLE_N; i++) {
                        p->pos[i] = p->p;
                }
        }
}

// called at half update rate!
static void tileanimations_update()
{
        i32 tick = os_tick();
        for (int n = 0; n < GAME_NUM_TILEANIMATIONS; n++) {
                tile_animation_s *a = &g_tileanimations[n];
                if (a->ticks == 0) continue;
                int frame        = (tick / a->ticks) % a->frames;
                g_tileIDs[a->ID] = a->IDs[frame];
        }
}