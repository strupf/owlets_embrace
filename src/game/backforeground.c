// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "backforeground.h"
#include "game.h"

enum {
        BACKGROUND_WIND_CIRCLE_R = 1900,
};

static void background_update_clouds(game_s *g, backforeground_s *bg)
{
        for (int n = bg->nclouds - 1; n >= 0; n--) {
                cloudbg_s *c = &bg->clouds[n];
                c->p.x += c->v;

                if ((c->v > 0 && c->p.x > (g->pixel_x << 8)) ||
                    (c->v < 0 && c->p.x < 0)) {
                        bg->clouds[n] = bg->clouds[--bg->nclouds];
                }
        }

        if (bg->nclouds < BG_NUM_CLOUDS && (os_tick() & 63) == 0) {
                cloudbg_s *c = &bg->clouds[bg->nclouds++];
                c->cloudtype = rng_max_u32(BG_NUM_CLOUD_TYPES - 1);
                c->v         = bg->clouddirection * rng_range(1, 64);
                c->p.y       = rng_range(0, (g->pixel_y << 8));

                if (c->v < 0) {
                        c->p.x = (g->pixel_x + 50) << 8;
                } else {
                        c->p.x = -(50 << 8);
                }
        }
}

static void background_update_wind_particles(game_s *g, backforeground_s *bg)
{

        // traverse backwards to avoid weird removal while iterating
        for (int n = bg->nparticles - 1; n >= 0; n--) {
                particlebg_s *p = &bg->particles[n];
                p->circcooldown--;
                if (p->circcooldown <= 0 && rng_fast_u16() < 1200) { // enter wind circle animation
                        p->ticks        = rng_range_u32(20, 50);
                        p->circticks    = p->ticks;
                        p->circc.x      = p->p.x;
                        p->circc.y      = p->p.y - BACKGROUND_WIND_CIRCLE_R;
                        p->circcooldown = p->circticks + 150;
                }

                if (p->circticks > 0) { // run through circle but keep slowly moving forward
                        i32 a  = (Q16_ANGLE_TURN * (p->ticks - p->circticks)) / p->ticks;
                        p->p.x = p->circc.x + ((sin_q16(a) * BACKGROUND_WIND_CIRCLE_R) >> 16);
                        p->p.y = p->circc.y + ((cos_q16(a) * BACKGROUND_WIND_CIRCLE_R) >> 16);
                        p->circc.x += 150;
                        p->circticks--;
                } else {
                        p->p = v2_add(p->p, p->v);
                        p->v.y += rng_range(-40, +40);
                        p->v.y = CLAMP(p->v.y, -230, +230);
                }

                p->pos[p->n] = p->p;
                p->n         = (p->n + 1) & (BG_WIND_PARTICLE_N - 1);

                if ((p->p.x >> 8) < -50 || (p->p.x >> 8) > g->pixel_x + 50) {
                        bg->particles[n] = bg->particles[--bg->nparticles];
                }
        }

        if (bg->nparticles < BG_NUM_PARTICLES && (os_tick() & 31) == 0) {
                particlebg_s *p = &bg->particles[bg->nparticles++];
                p->p.x          = -(10 << 8);
                p->p.y          = rng_range(0, g->pixel_y) << 8;
                p->v.x          = rng_range(1000, 2000);
                p->v.y          = 0;
                p->circcooldown = 10;
                p->circticks    = 0;
                p->n            = 0;
                for (int i = 0; i < BG_WIND_PARTICLE_N; i++) {
                        p->pos[i] = p->p;
                }
        }
}

void background_foreground_animate(game_s *g)
{
        backforeground_s *bg = &g->backforeground;
        background_update_clouds(g, bg);
        background_update_wind_particles(g, bg);
}