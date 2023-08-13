// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

u16              g_tileIDs[0x10000];
tile_animation_s g_tileanimations[NUM_TILEANIMATIONS];

static void tileanimations_update();
static void game_cull_scheduled(game_s *g);
static void game_update_transition(game_s *g);

roomdesc_s *roomlayout_get(roomlayout_s *rl, rec_i32 rec)
{
        for (int n = 0; n < rl->n_rooms; n++) {
                roomdesc_s *rd = &rl->rooms[n];
                if (rd != rl->curr && overlap_rec_excl(rd->r, rec))
                        return rd;
        }
        return NULL;
}

void roomlayout_load(roomlayout_s *rl, const char *filename)
{
        rl->n_rooms = 0;
        rl->curr    = NULL;
        os_spmem_push();

        char *txt = txt_read_file_alloc(filename, os_spmem_alloc);
        jsn_s jroot;
        jsn_root(txt, &jroot);
        foreach_jsn_childk (jroot, "maps", jmap) {
                roomdesc_s *rd = &rl->rooms[rl->n_rooms++];
                jsn_strk(jmap, "fileName", rd->filename, sizeof(rd->filename));
                rd->r.x = jsn_intk(jmap, "x");
                rd->r.y = jsn_intk(jmap, "y");
                rd->r.w = jsn_intk(jmap, "width");
                rd->r.h = jsn_intk(jmap, "height");
        }

        os_spmem_pop();
}

void door_think(game_s *g, obj_s *o)
{
        if (o->door.moved > 0) {
                solid_move(g, o, 0, -1);
                o->door.moved--;
                if (o->door.moved == 0) {
                        objflags_s flags = o->flags;
                        flags            = objflags_unset(flags,
                                                          OBJ_FLAG_THINK_1);
                        obj_set_flags(g, o, flags);
                }
        }
}

void doortrigger(game_s *g, obj_s *o, int triggerID, void *arg)
{
        if (o->ID == triggerID && !o->door.triggered) {
                // obj_delete(g, o);
                o->door.triggered = 1;
                objflags_s flags  = o->flags;
                flags             = objflags_set(flags,
                                                 OBJ_FLAG_THINK_1);
                obj_set_flags(g, o, flags);
                o->think_1    = door_think;
                o->door.moved = 100;
        }
}

obj_s *door_create(game_s *g)
{
        obj_s     *o     = obj_create(g);
        objflags_s flags = objflags_create(
            OBJ_FLAG_SOLID);
        obj_set_flags(g, o, flags);
        o->pos.x     = 340;
        o->pos.y     = 192 - 64 - 16;
        o->w         = 16;
        o->h         = 64 + 16;
        o->ontrigger = doortrigger;
        o->ID        = 4;
        return o;
}

void solid_think(game_s *g, obj_s *o)
{
        obj_s *solid = o;
        if (solid->pos.x > solid->p2 - 100) {
                solid->dir = -ABS(solid->dir);
        }
        if (solid->pos.x < solid->p1 + 32) {
                solid->dir = +ABS(solid->dir);
        }

        solid_move(g, solid, solid->dir, 0);
}

void background_animate(game_s *g)
{
        background_s *bg = &g->background;
        for (int n = 0; n < bg->nclouds; n++) {
                cloudbg_s *c = &bg->clouds[n];
                c->pos.x += c->velx;
        }
}

static void game_tick(game_s *g)
{
        if (debug_inp_enter()) {
                game_trigger(g, 4, NULL);
        }

        obj_listc_s thinkers = objbucket_list(g, OBJ_BUCKET_THINK_1);
        for (int i = 0; i < thinkers.n; i++) {
                obj_s *o = thinkers.o[i];
                if (o->think_1) o->think_1(g, o, o->userarg);
        }

        obj_listc_s movactors = objbucket_list(g, OBJ_BUCKET_MOVABLE_ACTOR);
        for (int i = 0; i < movactors.n; i++) {
                obj_s *o = movactors.o[i];
                obj_apply_movement(o);
                v2_i32 dt = v2_sub(o->pos_new, o->pos);
                actor_move_x(g, o, dt.x);
                actor_move_y(g, o, dt.y);
        }

        obj_listc_s okilloff   = objbucket_list(g, OBJ_BUCKET_KILL_OFFSCREEN);
        rec_i32     roffscreen = {-16, -16, g->pixel_x + 16, g->pixel_y + 16};
        for (int n = 0; n < okilloff.n; n++) {
                obj_s *o = okilloff.o[n];
                if (!overlap_rec_excl(roffscreen, obj_aabb(o))) {
                        obj_delete(g, o);
                }
        }

        // remove all objects scheduled to be deleted
        if (objset_len(&g->obj_scheduled_delete) > 0) {
                game_cull_scheduled(g);
        }

        cam_update(g, &g->cam);
        tileanimations_update();

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

        background_animate(g);
}

void game_update(game_s *g)
{
        static int once = 0;
        if (!once) {
                once = 1;
                roomlayout_load(&g->roomlayout, "assets/map/ww.world");
                g->roomlayout.curr = &g->roomlayout.rooms[0];
                door_create(g);
        }

        if (g->textbox.active) {
                textbox_s *tb = &g->textbox;
                textbox_update(tb);
                if (os_inp_just_pressed(INP_A) && g->textbox.shows_all) {
                        textbox_next_page(tb);
                }
                return;
        }

        if (g->transition.phase) {
                game_update_transition(g);
                return;
        }

        game_tick(g);
}

void game_trigger(game_s *g, int triggerID, void *arg)
{
        obj_listc_s lc = objbucket_list(g, OBJ_BUCKET_ALIVE);
        for (int n = 0; n < lc.n; n++) {
                obj_s *o = lc.o[n];
                if (o->ontrigger) {
                        o->ontrigger(g, o, triggerID, arg);
                }
        }
}

void game_close(game_s *g)
{
}

void game_map_transition_start(game_s *g, const char *filename)
{
        transition_s *t = &g->transition;
        if (t->phase) return;
        t->phase = TRANSITION_FADE_IN;
        t->ticks = 0;

        os_strcpy(t->map, filename);
}

static void game_update_transition(game_s *g)
{
        transition_s *t = &g->transition;
        t->ticks++;
        if (t->ticks < TRANSITION_TICKS)
                return;

        switch (t->phase) {
        case TRANSITION_FADE_IN: {
                char filename[64] = {0};
                os_strcat(filename, ASSET_PATH_MAPS);
                os_strcat(filename, t->map);
                game_load_map(g, filename);
                obj_s *ohero;
                if (t->enterfrom) {
                        try_obj_from_handle(g->hero.obj, &ohero);
                        g->cam        = t->camprev;
                        v2_i32 offset = {
                            g->cam.pos.x - t->heroprev.x,
                            g->cam.pos.y - t->heroprev.y};
                        ohero->pos.x = t->heroprev.x;
                        ohero->pos.y = t->heroprev.y;

                        switch (t->enterfrom) {
                        case 0: break;
                        case DIRECTION_W:
                                ohero->pos.x = g->pixel_x - ohero->w;
                                break;
                        case DIRECTION_E:
                                ohero->pos.x = 0;
                                break;
                        case DIRECTION_N:
                                ohero->pos.y = g->pixel_y - ohero->h;
                                break;
                        case DIRECTION_S:
                                ohero->pos.y = 0;
                                break;
                        }
                        g->cam.pos = v2_add(ohero->pos, offset);
                        cam_constrain_to_room(g, &g->cam);
                }

                t->phase = TRANSITION_FADE_OUT;
                t->ticks = 0;
        } break;
        case TRANSITION_FADE_OUT:
                t->phase = TRANSITION_NONE;
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