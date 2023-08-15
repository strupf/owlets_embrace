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

static void game_tick(game_s *g)
{
        if (debug_inp_enter()) {
                game_trigger(g, 4, NULL);
        }

        obj_listc_s thinkers1 = objbucket_list(g, OBJ_BUCKET_THINK_1);
        for (int i = 0; i < thinkers1.n; i++) {
                obj_s *o = thinkers1.o[i];
                o->think_1(g, o, o->userarg);
        }

        obj_listc_s movactors = objbucket_list(g, OBJ_BUCKET_MOVABLE_ACTOR);
        for (int i = 0; i < movactors.n; i++) {
                obj_s *o = movactors.o[i];
                obj_apply_movement(o);
                v2_i32 dt = v2_sub(o->pos_new, o->pos);
                actor_move_x(g, o, dt.x);
                actor_move_y(g, o, dt.y);
        }

        obj_listc_s thinkers2 = objbucket_list(g, OBJ_BUCKET_THINK_2);
        for (int i = 0; i < thinkers2.n; i++) {
                obj_s *o = thinkers2.o[i];
                o->think_2(g, o, o->userarg);
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

        background_foreground_animate(g);
}

static void textbox_input(game_s *g, textbox_s *tb)
{
        textbox_update(tb);
        if (!tb->shows_all) return;

        if (os_inp_just_pressed(INP_A)) {
                if (tb->n_choices) {
                        textbox_select_choice(g, tb, tb->cur_choice);
                } else {
                        textbox_next_page(tb);
                }
        } else if (tb->n_choices) {
                if (os_inp_just_pressed(INP_DOWN))
                        tb->cur_choice++;
                if (os_inp_just_pressed(INP_UP))
                        tb->cur_choice--;

                if (tb->cur_choice < 0)
                        tb->cur_choice = tb->n_choices - 1;
                if (tb->cur_choice >= tb->n_choices)
                        tb->cur_choice = 0;
        }
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
                textbox_input(g, tb);

                return;
        }

        if (g->transition.phase) {
                game_update_transition(g);
                return;
        }

        game_tick(g);
}

void game_trigger(game_s *g, int triggerID)
{
        obj_listc_s lc = objbucket_list(g, OBJ_BUCKET_ALIVE);
        for (int n = 0; n < lc.n; n++) {
                obj_s *o = lc.o[n];
                if (o->ontrigger) {
                        o->ontrigger(g, o, triggerID);
                }
        }
}

void game_close(game_s *g)
{
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

bool32 solid_occupies(obj_s *solid, rec_i32 r)
{
        return overlap_rec_excl(obj_aabb(solid), r);
}

bool32 game_area_blocked(game_s *g, rec_i32 r)
{
        if (tiles_area(tilegrid_from_game(g), r)) return 1;
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

particle_s *particle_spawn(game_s *g)
{
        ASSERT(g->n_particles < NUM_PARTICLES);

        particle_s *p = &g->particles[g->n_particles++];
        *p            = (const particle_s){0};
        return p;
}

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

void obj_interact_dialog(game_s *g, obj_s *o, void *arg)
{
        textbox_load_dialog(&g->textbox, o->filename);
}