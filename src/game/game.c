// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

u16              g_tileIDs[0x10000];
tile_animation_s g_tileanimations[NUM_TILEANIMATIONS];

void pathmover_init(pathmover_s *p)
{
        for (int n = 0; n < 5; n++) {
                p->nodes[n].p = (v2_i32){rng_range(100, 400), rng_range(10, 210)};
                if (n > 0) {
                        p->nodes[n].prev = &p->nodes[n - 1];
                }
                if (n < 5 - 1) {
                        p->nodes[n].next = &p->nodes[n + 1];
                }
        }

        // p->nodes[0].prev = &p->nodes[4];
        // p->nodes[4].next = &p->nodes[0];
        p->movetype = PATH_LINE_PINGPONG;
        p->pathtype = PATH_TYPE_LINE;
        p->v        = 1;
        p->from     = &p->nodes[0];
        p->to       = &p->nodes[1];
}

void solid_think(game_s *g, obj_s *o)
{
        if (os_tick() & 1) return;
        obj_s *solid = o;
        if (solid->pos.x > solid->p2) {
                solid->dir = -ABS(solid->dir);
        }
        if (solid->pos.x < solid->p1) {
                solid->dir = +ABS(solid->dir);
        }
        solid->tomove.x += solid->dir;
}

static void game_tick(game_s *g)
{
        static int ponce = 0;
        if (!ponce) {
                ponce = 1;
                pathmover_init(&g->pathmover);
                // blob_create(g);
        }

        path_update(&g->pathmover);
        water_update(&g->water);
        ocean_update(&g->ocean);
        if (debug_inp_enter()) {
                water_impact(&g->water, 50, 10, 30000);
        }
        obj_listc_s thinkers1 = objbucket_list(g, OBJ_BUCKET_THINK_1);
        for (int i = 0; i < thinkers1.n; i++) {
                o->think_1(g, thinkers1.o[i]);
        }

        obj_listc_s movables = objbucket_list(g, OBJ_BUCKET_MOVABLE);
        for (int i = 0; i < movables.n; i++) {
                obj_apply_movement(movables.o[i]);
        }

        obj_listc_s actors = objbucket_list(g, OBJ_BUCKET_ACTOR);
        for (int i = 0; i < actors.n; i++) {
                actors.o[i]->actorres = 0;
        }

        obj_listc_s solids = objbucket_list(g, OBJ_BUCKET_SOLID);
        for (int i = 0; i < solids.n; i++) {
                obj_s *o = solids.o[i];
                solid_move(g, o, o->tomove.x, o->tomove.y);
                o->tomove.x = 0;
                o->tomove.y = 0;
        }

        for (int i = 0; i < actors.n; i++) {
                obj_s *o = actors.o[i];
                actor_move(g, o, o->tomove.x, o->tomove.y);
                o->tomove.x = 0;
                o->tomove.y = 0;
        }

        obj_listc_s enemyhurt = objbucket_list(g, OBJ_BUCKET_HURTS_ENEMIES);
        obj_listc_s enemies   = objbucket_list(g, OBJ_BUCKET_ENEMY);
        for (int n = 0; n < enemyhurt.n; n++) {
                obj_s *o = enemyhurt.o[n];
                for (int i = 0; i < enemies.n; i++) {
                        obj_s *e = enemies.o[i];
                        if (overlap_rec_excl(obj_aabb(o), obj_aabb(e))) {
                                obj_delete(g, e);
                                o->hit_enemy = 1;

                                for (int i = 0; i < 60; i++) {
                                        particle_s *particle = particle_spawn(g);
                                        particle->ticks      = rng_range(15, 25);
                                        particle->p_q8       = (v2_i32){o->pos.x + o->w / 2,
                                                                        o->pos.y + o->h - 4};
                                        particle->p_q8       = v2_shl(particle->p_q8, 8);

                                        particle->p_q8.x += rng_range(-800, 800);
                                        particle->p_q8.y += rng_range(-800, 800);

                                        particle->v_q8.x = rng_range(-300, 300);
                                        particle->v_q8.y = rng_range(-300, 300);
                                        particle->a_q8.y = 30;
                                }
                        }
                }
        }

        obj_listc_s thinkers2 = objbucket_list(g, OBJ_BUCKET_THINK_2);
        for (int i = 0; i < thinkers2.n; i++) {
                obj_s *o = thinkers2.o[i];
                o->think_2(g, o);
        }

        obj_listc_s okilloff   = objbucket_list(g, OBJ_BUCKET_KILL_OFFSCREEN);
        rec_i32     roffscreen = {-256, -256, g->pixel_x + 256, g->pixel_y + 256};
        for (int n = 0; n < okilloff.n; n++) {
                obj_s *o = okilloff.o[n];
                if (!overlap_rec_excl(roffscreen, obj_aabb(o))) {
                        obj_delete(g, o);
                }
        }

        // remove all objects scheduled to be deleted
        game_cull_scheduled(g);

        obj_listc_s animators = objbucket_list(g, OBJ_BUCKET_ANIMATE);
        for (int n = 0; n < animators.n; n++) {
                obj_s *o = animators.o[n];
                ASSERT(o->animate_func);
                o->animate_func(g, o);
        }

        obj_listc_s spriteanim = objbucket_list(g, OBJ_BUCKET_SPRITE_ANIM);
        for (int n = 0; n < spriteanim.n; n++) {
                obj_s *o = spriteanim.o[n];
                for (int i = 0; i < o->nspriteanim; i++) {
                        sprite_anim_s *sa = &(o->spriteanim[i].a);
                        sprite_anim_update(sa);
                }
        }

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

static void update_gameplay(game_s *g)
{
        static int once = 0;
        if (!once) {
                once = 1;
                roomlayout_load(&g->roomlayout, "assets/map/ww.world");
                g->roomlayout.curr = &g->roomlayout.rooms[0];
                door_create(g);
        }

        if (os_tick() & 1) {
                tileanimations_update();
        } else {
                backforeground_animate(g);
        }

        if (g->areaname_display_ticks > 0)
                g->areaname_display_ticks--;

        bool32     gameupdate = 1;
        textbox_s *tb         = &g->textbox;
        g->hero.caninteract   = (textbox_state(tb) == TEXTBOX_STATE_INACTIVE);
        if (textbox_state(tb) != TEXTBOX_STATE_INACTIVE) {
                textbox_update(g, tb);
                if (textbox_blocking(tb)) {
                        textbox_input(g, tb);
                        gameupdate = 0;
                }
        } else if (g->transition.phase) {
                game_update_transition(g);
                gameupdate = 0;
        }

        if (gameupdate)
                game_tick(g);

        cam_update(g, &g->cam);
        if (os_inp_crankp() != os_inp_crank())
                g->hero.itemselection_dirty = 1;
}

static void update_title(game_s *g)
{
}

void game_update(game_s *g)
{
        switch (g->state) {
        case GAMESTATE_TITLE: {
                update_title(g);
        } break;
        case GAMESTATE_GAMEPLAY: {
                update_gameplay(g);
        } break;
        }
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

void *game_heapalloc(game_s *g, size_t size)
{
        return memheap_alloc(&g->heap, size);
}

void game_heapfree(game_s *g, void *ptr)
{
        memheap_free(&g->heap, ptr);
}

// called at half update rate!
void tileanimations_update()
{
        i32 tick = os_tick();
        for (int n = 0; n < NUM_TILEANIMATIONS; n++) {
                tile_animation_s *a = &g_tileanimations[n];
                if (a->ticks == 0) continue;
                int frame        = (tick / a->ticks) % a->frames;
                g_tileIDs[a->ID] = a->IDs[frame];
        }
}

void game_cull_scheduled(game_s *g)
{
        int len = objset_len(&g->obj_scheduled_delete);
        if (len == 0) return;
        for (int n = 0; n < len; n++) {
                obj_s *o_del = objset_at(&g->obj_scheduled_delete, n);
                for (int i = 0; i < NUM_OBJ_BUCKETS; i++) {
                        objset_del(&g->objbuckets[i].set, o_del);
                }
                o_del->gen++; // invalidate existing handles
                g->objfreestack[g->n_objfree++] = o_del;
        }
        objset_clr(&g->obj_scheduled_delete);
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

bool32 game_is_ladder(game_s *g, v2_i32 p)
{
        if (!(0 <= p.x && p.x < g->pixel_x && 0 <= p.y && p.y < g->pixel_y))
                return 0;
        return g->tiles[(p.x >> 4) + (p.y >> 4) * g->tiles_x] == TILE_LADDER;
}

obj_listc_s objbucket_list(game_s *g, int bucketID)
{
        ASSERT(0 <= bucketID && bucketID < NUM_OBJ_BUCKETS);
        return objset_list(&g->objbuckets[bucketID].set);
}

particle_s *particle_spawn(game_s *g)
{
        if (g->n_particles < NUM_PARTICLES) {
                particle_s *p = &g->particles[g->n_particles++];
                *p            = (const particle_s){0};
                return p;
        }
        return NULL;
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
