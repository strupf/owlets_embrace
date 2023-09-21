// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "transition.h"
#include "game.h"

static void transition_start(game_s *g, char *filename, v2_i32 location, int dir_slide)
{
        transition_s *t = &g->transition;
        t->phase        = TRANSITION_FADE_IN;
        t->ticks        = 0;
        t->teleportto   = location;
        t->dir_slide    = dir_slide;
        os_strcpy(t->map, filename);
}

void transition_update(game_s *g)
{
        transition_s *t = &g->transition;
        switch (t->phase) {
        case TRANSITION_NONE: break;
        case TRANSITION_FADE_IN: {
                if (++t->ticks < TRANSITION_TICKS) break;
                t->phase = TRANSITION_FADE_BLACK;
                t->ticks = 0;
                game_load_map(g, t->map);
                obj_s *ohero;
                try_obj_from_handle(g->hero.obj, &ohero);
                ohero->pos = t->teleportto;
                if (t->dir_slide == DIRECTION_N)
                        ohero->vel_q8.y = -2 * HERO_C_JUMP_INIT;
                g->cam.pos = obj_aabb_center(ohero);
                cam_constrain_to_room(g, &g->cam);
        } break;
        case TRANSITION_FADE_BLACK:
                if (++t->ticks < TRANSITION_BLACK_TICKS) break;
                t->phase = TRANSITION_FADE_OUT;
                t->ticks = 0;
                break;
        case TRANSITION_FADE_OUT:
                if (++t->ticks < TRANSITION_TICKS) break;
                t->phase = TRANSITION_NONE;
                t->ticks = 0;
                break;
        }
}