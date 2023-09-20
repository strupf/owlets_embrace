// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

u16              g_tileIDs[0x10000];
tile_animation_s g_tileanimations[0x10000];

static void game_tick(game_s *g)
{
        // water_update(&g->water);
        // ocean_update(&g->ocean);
        if (debug_inp_enter()) {
                // water_impact(&g->water, 50, 10, 30000);
        }
        obj_listc_s thinkers1 = objbucket_list(g, OBJ_BUCKET_THINK_1);
        for (int i = 0; i < thinkers1.n; i++) {
                obj_s *o = thinkers1.o[i];
                o->think_1(g, o);
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

        obj_listc_s okilloff   = objbucket_list(g, OBJ_BUCKET_KILL_OFFSCREEN);
        rec_i32     roffscreen = {-256, -256, g->pixel_x + 256, g->pixel_y + 256};
        for (int n = 0; n < okilloff.n; n++) {
                obj_s *o = okilloff.o[n];
                if (!overlap_rec_excl(roffscreen, obj_aabb(o))) {
                        obj_delete(g, o);
                }
        }

        game_obj_group_collisions(g);

        obj_listc_s thinkers2 = objbucket_list(g, OBJ_BUCKET_THINK_2);
        for (int i = 0; i < thinkers2.n; i++) {
                obj_s *o = thinkers2.o[i];
                o->think_2(g, o);
        }

        // remove all objects scheduled to be deleted
        game_cull_scheduled(g);

        obj_listc_s animators = objbucket_list(g, OBJ_BUCKET_ANIMATE);
        for (int n = 0; n < animators.n; n++) {
                obj_s *o = animators.o[n];
                o->animate_func(g, o);
        }

        obj_listc_s spriteanim = objbucket_list(g, OBJ_BUCKET_SPRITE_ANIM);
        for (int n = 0; n < spriteanim.n; n++) {
                obj_s *o = spriteanim.o[n];
        }
}

static void update_gameplay(game_s *g)
{
        g->area_name_ticks--;

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
                transition_update(g);
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

// naive brute force obj collisions
void game_obj_group_collisions(game_s *g)
{
        os_spmem_push();

        obj_pair_s *pairs  = (obj_pair_s *)os_spmem_alloc(sizeof(obj_pair_s) * NUM_OBJS * NUM_OBJS);
        int         npairs = 0;
        obj_listc_s l      = objbucket_list(g, OBJ_BUCKET_ALIVE);
        for (int i = 0; i < l.n; i++) {
                obj_s  *oi = l.o[i];
                rec_i32 ri = obj_aabb(oi);
                for (int j = i + 1; j < l.n; j++) {
                        obj_s  *oj = l.o[j];
                        rec_i32 rj = obj_aabb(oj);
                        if (!overlap_rec_excl(ri, rj)) continue;
                        obj_pair_s pair = {oi, oj};
                        pairs[npairs++] = pair;
                }
        }

        os_spmem_pop();
}

savepoint_s *game_create_savepoint(game_s *g)
{
        ASSERT(g->n_savepoints < ARRLEN(g->savepoints));
        return &(g->savepoints[g->n_savepoints++]);
}

particle_s *particle_spawn(game_s *g)
{
        if (g->n_particles < 256) {
                particle_s *p = &g->particles[g->n_particles++];
                *p            = (const particle_s){0};
                return p;
        }
        return NULL;
}