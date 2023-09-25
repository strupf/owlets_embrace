// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "transition.h"
#include "game.h"

static void cb_transition_finish(void *arg)
{
        game_s *g                = (game_s *)arg;
        g->transition.inprogress = 0;
}

static void cb_transition_load(void *arg)
{
        game_s       *g = (game_s *)arg;
        transition_s *t = &g->transition;
        game_load_map(g, t->map);
        obj_s *ohero;
        try_obj_from_handle(g->hero.obj, &ohero);
        ohero->pos    = t->teleportto;
        ohero->vel_q8 = t->vel;
        if (t->dir_slide == DIRECTION_N)
                ohero->vel_q8.y = -2 * HERO_C_JUMP_INIT;
        g->cam.pos = obj_aabb_center(ohero);
        cam_constrain_to_room(g, &g->cam);
}

static void transition_start(game_s *g, char *filename, v2_i32 location, int dir_slide)
{
        transition_s *t = &g->transition;
        t->inprogress   = 1;
        fading_start(&g->global_fade, 15, 10, 15,
                     cb_transition_load,
                     cb_transition_finish, g);
        t->teleportto = location;
        t->dir_slide  = dir_slide;
        if (dir_slide) {
                t->vel = g->hero.obj.o->vel_q8;
        } else {
                t->vel = (v2_i32){0};
        }
        os_strcpy(t->map, filename);
}

bool32 transition_active(game_s *g)
{
        return g->transition.inprogress;
}