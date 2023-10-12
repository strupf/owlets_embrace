// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "backforeground.h"
#include "game.h"

// called at half update rate!
static void       backforeground_tileanimations(game_s *g, backforeground_s *b);
static cloudbg_s *backforeground_spawn_cloud(game_s *g, backforeground_s *b);
static void       backforeground_clouds(game_s *g, backforeground_s *b);
static void       backforeground_windparticles(game_s *g, backforeground_s *b);
static void       backforeground_animate_grass(game_s *g, backforeground_s *b);

void backforeground_setup(game_s *g, backforeground_s *b)
{
        for (int n = 0; n < 16; n++) {
                cloudbg_s *c = backforeground_spawn_cloud(g, b);
                c->p.x       = rng_range(-100, 400 + 100) << 8;
                c->p.y       = rng_range(-100, 240) << 8;
        }
}

void backforeground_animate(game_s *g, backforeground_s *b)
{
        backforeground_tileanimations(g, b);
        backforeground_clouds(g, b);
        backforeground_windparticles(g, b);
        backforeground_animate_grass(g, b);
        for (int n = b->n_particles - 1; n >= 0; n--) {
                particle_s *p = &b->particles[n];
                if (--p->ticks == 0) {
                        b->particles[n] = b->particles[--b->n_particles];
                        continue;
                }
                p->v_q8 = v2_add(p->v_q8, p->a_q8);
                p->p_q8 = v2_add(p->p_q8, p->v_q8);
        }
}

static void backforeground_animate_grass(game_s *g, backforeground_s *b)
{
        obj_listc_s actors = objbucket_list(g, OBJ_BUCKET_ACTOR);

        for (int n = 0; n < b->n_grass; n++) {
                grass_s *gr = &b->grass[n];
                int      f1 = -((gr->x_q8 * 4) >> 8);
                int      f2 = (rng_fast_i16() * 150) >> (13 + 8);
                gr->v_q8 += f1 + f2;

                rec_i32 rgrass = {gr->pos.x, gr->pos.y, 16, 16};
                for (int i = 0; i < actors.n; i++) {
                        obj_s *o = actors.o[i];
                        if (overlap_rec_excl(rgrass, obj_aabb(o))) {
                                gr->v_q8 += o->vel_q8.x >> 6;
                        }
                }

                gr->x_q8 += gr->v_q8;
                gr->x_q8 = clamp_i(gr->x_q8,
                                   -BG_PLANTS_MAX_EXCURSION_Q8,
                                   +BG_PLANTS_MAX_EXCURSION_Q8);
                gr->v_q8 = (gr->v_q8 * 245) >> 8;
        }
}

static cloudbg_s *backforeground_spawn_cloud(game_s *g, backforeground_s *b)
{
        ASSERT(b->n_clouds < BG_NUM_CLOUDS);
        cloudbg_s *c = &b->clouds[b->n_clouds++];
        c->cloudtype = rng_max_u32(BG_NUM_CLOUD_TYPES - 1);
        c->v         = b->clouddirection * rng_range(4, 16);
        c->p.y       = rng_range(-50, 240) << 8;

        if (c->v < 0) {
                c->p.x = (400 + 100) << 8;
        } else {
                c->p.x = -(100 << 8);
        }
        return c;
}

static void backforeground_clouds(game_s *g, backforeground_s *b)
{
        for (int n = b->n_clouds - 1; n >= 0; n--) {
                cloudbg_s *c = &b->clouds[n];
                c->p.x += c->v;

                if ((c->v > 0 && c->p.x > (g->pixel_x << 8)) ||
                    (c->v < 0 && c->p.x < 0)) {
                        b->clouds[n] = b->clouds[--b->n_clouds];
                }
        }

        if (b->n_clouds < BG_NUM_CLOUDS && rng_fast_u16() <= 200) {
                cloudbg_s *c = backforeground_spawn_cloud(g, b);
        }
}

static void backforeground_windparticles(game_s *g, backforeground_s *b)
{
        // traverse backwards to avoid weird removal while iterating
        for (int n = b->n_windparticles - 1; n >= 0; n--) {
                windparticle_s *p = &b->windparticles[n];
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
                        b->windparticles[n] = b->windparticles[--b->n_windparticles];
                }
        }

        if (b->n_windparticles < BG_NUM_PARTICLES && rng_fast_u16() <= 4000) {
                windparticle_s *p = &b->windparticles[b->n_windparticles++];
                p->p.x            = -(10 << 8);
                p->p.y            = rng_range(0, g->pixel_y) << 8;
                p->v.x            = rng_range(2000, 4000);
                p->v.y            = 0;
                p->circcooldown   = 10;
                p->circticks      = 0;
                p->n              = 0;
                for (int i = 0; i < BG_WIND_PARTICLE_N; i++) {
                        p->pos[i] = p->p;
                }
        }
}

// called at half update rate!
static void backforeground_tileanimations(game_s *g, backforeground_s *b)
{
        i32 tick = os_tick();
        for (int n = 0; n < GAME_NUM_TILEANIMATIONS; n++) {
                tile_animation_s *a = &g_tileanimations[n];
                if (a->ticks == 0) continue;
                int frame        = (tick / a->ticks) % a->frames;
                g_tileIDs[a->ID] = a->IDs[frame];
        }
}

void backforeground_spawn_particles(backforeground_s *b, bgpartice_desc_s d)
{
        for (int n = 0; n < d.n; n++) {
                if (b->n_particles >= NUM_PARTICLES) break;

                particle_s *p = &b->particles[b->n_particles++];
                p->p_q8       = d.p_q8;
                p->p_q8.x += rng_range(-d.p_q8_spread.x, +d.p_q8_spread.x);
                p->p_q8.y += rng_range(-d.p_q8_spread.y, +d.p_q8_spread.y);

                p->v_q8 = d.v_q8;
                p->v_q8.x += rng_range(-d.v_q8_spread.x, +d.v_q8_spread.x);
                p->v_q8.y += rng_range(-d.v_q8_spread.y, +d.v_q8_spread.y);

                p->a_q8 = d.a_q8;
                p->a_q8.x += rng_range(-d.a_q8_spread.x, +d.a_q8_spread.x);
                p->a_q8.y += rng_range(-d.a_q8_spread.y, +d.a_q8_spread.y);

                p->ticks    = d.tick + rng_range(-d.tick_spread, +d.tick_spread);
                p->ticks_og = p->ticks;
                p->size     = d.size;
        }
}