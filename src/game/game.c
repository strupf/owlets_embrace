// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

game_s           g_gamestate;
u16              g_tileIDs[GAME_NUM_TILEIDS];
tile_animation_s g_tileanimations[GAME_NUM_TILEANIMATIONS];
/* contains triangles for each tile describing its shape.
 */
const tri_i32    tilecolliders[GAME_NUM_TILECOLLIDERS] = {
    // dummy triangles
    0, 0, 0, 0, 0, 0, // empty
    0, 0, 0, 0, 0, 0, // solid
    // slope 45
    0, 16, 16, 16, 16, 0, // 2
    0, 0, 16, 0, 16, 16,  // 3
    0, 0, 0, 16, 16, 16,  // 4
    0, 0, 0, 16, 16, 0,   // 5
    // slope lo
    0, 16, 16, 16, 16, 8, // 6
    0, 0, 16, 0, 16, 8,   // 7
    0, 8, 0, 16, 16, 16,  // 8
    0, 0, 16, 0, 0, 8,    // 9
    16, 0, 16, 16, 8, 16, // 10
    8, 0, 16, 0, 16, 16,  // 11
    0, 0, 0, 16, 8, 16,   // 12
    0, 0, 8, 0, 0, 16,    // 13
    // slopes hi
    0, 16, 16, 16, 16, 8, // 14
    0, 16, 16, 16, 16, 8,
    0, 0, 16, 0, 16, 8, // 15
    0, 0, 16, 0, 16, 8,
    0, 8, 0, 16, 16, 16, // 16
    0, 8, 0, 16, 16, 16,
    0, 0, 16, 0, 0, 8, // 17
    0, 0, 16, 0, 0, 8,
    16, 0, 16, 16, 8, 16, // 18
    16, 0, 16, 16, 8, 16,
    8, 0, 16, 0, 16, 16, // 19
    8, 0, 16, 0, 16, 16,
    0, 0, 0, 16, 8, 16, // 20
    0, 0, 0, 16, 8, 16,
    0, 0, 8, 0, 0, 16, // 21
    0, 0, 8, 0, 0, 16,
    //
};

static void game_tick(game_s *g)
{
        game_savefile_delete(0);

        obj_listc_s thinkers1 = objbucket_list(g, OBJ_BUCKET_THINK_1);
        for (int i = 0; i < thinkers1.n; i++) {
                obj_s *o = thinkers1.o[i];
                o->think_1(g, o);
        }

        obj_listc_s movables = objbucket_list(g, OBJ_BUCKET_MOVABLE);
        for (int i = 0; i < movables.n; i++) {
                obj_apply_movement(movables.o[i]);
        }

        obj_listc_s solids = objbucket_list(g, OBJ_BUCKET_SOLID);
        for (int i = 0; i < solids.n; i++) {
                obj_s *o = solids.o[i];
                solid_move(g, o, o->tomove.x, o->tomove.y);
                o->tomove.x = 0;
                o->tomove.y = 0;
        }

        obj_listc_s actors = objbucket_list(g, OBJ_BUCKET_ACTOR);
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

        room_deco_animate(g);
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
        } else if (transition_active(g)) {
                gameupdate = 0;
        }

        if (gameupdate)
                game_tick(g);

        cam_update(g, &g->cam);

        for (int n = 1; n < NUM_OBJS; n++) { // check for overrides
                ASSERT(g->objs[n].magic == MAGIC_NUM_OBJ);
        }
}

void game_update(game_s *g)
{
        switch (g->state) {
        case GAMESTATE_TITLE: {
                update_title(g, &g->mainmenu);
        } break;
        case GAMESTATE_GAMEPLAY: {
                update_gameplay(g);
        } break;
        }

        fading_update(&g->global_fade);
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
        obj_listc_s l = objbucket_list(g, OBJ_BUCKET_ALIVE);
        for (int i = 0; i < l.n; i++) {
                l.o[i]->n_colliders = 0;
        }

        for (int i = 0; i < l.n; i++) {
                obj_s  *oi = l.o[i];
                rec_i32 ri = obj_aabb(oi);
                for (int j = i + 1; j < l.n; j++) {
                        obj_s  *oj = l.o[j];
                        rec_i32 rj = obj_aabb(oj);
                        if (!overlap_rec_excl(ri, rj)) continue;
                        oi->colliders[oi->n_colliders++] = oj;
                        oj->colliders[oj->n_colliders++] = oi;
                }
        }
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

void game_fade(game_s *g,
               int     ticks_fade_out,
               int     ticks_fade_black,
               int     ticks_fade_in,
               void(fadecb)(game_s *g, void *arg), void *arg)
{
}

bool32 game_fading(game_s *g)
{
}