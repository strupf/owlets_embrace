// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

u16              g_tileIDs[0x10000];
tile_animation_s g_tileanimations[NUM_TILEANIMATIONS];

static void tileanimations_update();
static void game_cull_scheduled(game_s *g);
static void game_update_transition(game_s *g);

void solid_think(game_s *g, obj_s *o)
{
        obj_s *solid = o;
        if (solid->pos.x > solid->p2) {
                solid->dir = -ABS(solid->dir);
        }
        if (solid->pos.x < solid->p1) {
                solid->dir = +ABS(solid->dir);
        }

        solid_move(g, solid, solid->dir, 0);
}

void game_update(game_s *g)
{
        textbox_s *tb = &g->textbox;
        if (tb->active) {
                textbox_update(tb);
                if (os_inp_just_pressed(INP_A) && g->textbox.shows_all) {
                        textbox_next_page(tb);
                }
                return;
        }

        obj_listc_s thinkers = objbucket_list(g, OBJ_BUCKET_THINK_1);
        for (int i = 0; i < thinkers.n; i++) {
                obj_s *o = thinkers.o[i];
                if (o->think_1) o->think_1(g, o);
        }

        obj_listc_s movactors = objbucket_list(g, OBJ_BUCKET_MOVABLE_ACTOR);
        for (int i = 0; i < movactors.n; i++) {
                obj_s *o = movactors.o[i];
                obj_apply_movement(o);
                v2_i32 dt = v2_sub(o->pos_new, o->pos);
                obj_move_x(g, o, dt.x);
                obj_move_y(g, o, dt.y);
        }

        // remove all objects scheduled to be deleted
        if (objset_len(&g->obj_scheduled_delete) > 0) {
                game_cull_scheduled(g);
        }

        cam_update(g, &g->cam);
        tileanimations_update();

        if (g->transitionphase) {
                game_update_transition(g);
        }

        for (int n = g->n_particles - 1; n >= 0; n--) {
                particle_s *p = &g->particles[n];
                p->ticks--;
                if (p->ticks == 0) {
                        g->particles[n] = g->particles[--g->n_particles];
                        continue;
                }
                p->v_q8 = v2_add(p->v_q8, p->a_q8);
                p->p_q8 = v2_add(p->p_q8, p->v_q8);
        }
}

void game_close(game_s *g)
{
}

void game_map_transition_start(game_s *g, const char *filename)
{
        if (g->transitionphase != 0) return;
        g->transitionphase = TRANSITION_FADE_IN;
        g->transitionticks = 0;

        os_strcpy(g->transitionmap, filename);
}

static void game_update_transition(game_s *g)
{
        g->transitionticks++;
        if (g->transitionticks < TRANSITION_TICKS)
                return;

        char filename[64] = {0};
        switch (g->transitionphase) {
        case TRANSITION_FADE_IN:

                os_strcat(filename, ASSET_PATH_MAPS);
                os_strcat(filename, g->transitionmap);
                game_load_map(g, filename);
                g->transitionphase = TRANSITION_FADE_OUT;
                g->transitionticks = 0;
                break;
        case TRANSITION_FADE_OUT:
                g->transitionphase = TRANSITION_NONE;
                break;
        }
}

static void tileanimations_update()
{
        i32 tick = os_tick();
        for (int n = 0; n < NUM_TILEANIMATIONS; n++) {
                tile_animation_s *a = &g_tileanimations[n];
                if (a->ticks == 0) continue;
                int frame        = 0;
                frame            = (tick / a->ticks) % a->frames;
                g_tileIDs[a->ID] = a->IDs[frame];
        }
}

static void game_cull_scheduled(game_s *g)
{
        for (int n = 0; n < objset_len(&g->obj_scheduled_delete); n++) {
                obj_s *o_del = objset_at(&g->obj_scheduled_delete, n);
                for (int i = 0; i < NUM_OBJ_BUCKETS; i++) {
                        objset_del(&g->objbuckets[i].set, o_del);
                }
                o_del->gen++; // invalidate existing handles
                g->objfreestack[g->n_objfree++] = o_del;
        }
        objset_clr(&g->obj_scheduled_delete);
}

tilegrid_s game_tilegrid(game_s *g)
{
        tilegrid_s tg =
            {g->tiles, g->tiles_x, g->tiles_y, g->pixel_x, g->pixel_y};
        return tg;
}

bool32 solid_occupies(obj_s *solid, rec_i32 r)
{
        return overlap_rec_excl(obj_aabb(solid), r);
}

bool32 game_area_blocked(game_s *g, rec_i32 r)
{
        if (tiles_area(game_tilegrid(g), r)) return 1;
        obj_listc_s solids = objbucket_list(g, OBJ_BUCKET_SOLID);
        for (int n = 0; n < solids.n; n++) {
                obj_s *o = solids.o[n];
                if (overlap_rec_excl(obj_aabb(o), r))
                        return 1;
        }
        return 0;
}

obj_listc_s objbucket_list(game_s *g, int bucketID)
{
        ASSERT(0 <= bucketID && bucketID < NUM_OBJ_BUCKETS);
        return objset_list(&g->objbuckets[bucketID].set);
}

void game_tile_bounds_minmax(game_s *g, v2_i32 pmin, v2_i32 pmax,
                             i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        *x1 = MAX(pmin.x, 0) >> 4; /* "/ 16" */
        *y1 = MAX(pmin.y, 0) >> 4;
        *x2 = MIN(pmax.x, g->pixel_x - 1) >> 4;
        *y2 = MIN(pmax.y, g->pixel_y - 1) >> 4;
}

void game_tile_bounds_tri(game_s *g, tri_i32 t,
                          i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        v2_i32 pmin = v2_min(t.p[0], v2_min(t.p[1], t.p[2]));
        v2_i32 pmax = v2_max(t.p[0], v2_max(t.p[1], t.p[2]));
        game_tile_bounds_minmax(g, pmin, pmax, x1, y1, x2, y2);
}

void game_tile_bounds_rec(game_s *g, rec_i32 r,
                          i32 *x1, i32 *y1, i32 *x2, i32 *y2)
{
        v2_i32 pmin = {r.x, r.y};
        v2_i32 pmax = {r.x + r.w, r.y + r.h};
        game_tile_bounds_minmax(g, pmin, pmax, x1, y1, x2, y2);
}

particle_s *particle_spawn(game_s *g)
{
        ASSERT(g->n_particles < NUM_PARTICLES);

        particle_s *p = &g->particles[g->n_particles++];
        *p            = (const particle_s){0};
        return p;
}