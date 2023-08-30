// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "maptransition.h"
#include "game.h"

void game_map_transition_start(game_s *g, const char *filename)
{
        transition_s *t = &g->transition;
        if (t->phase) return;
        t->phase = TRANSITION_FADE_IN;
        t->ticks = 0;

        os_strcpy(t->map, filename);
}

void game_update_transition(game_s *g)
{
        transition_s *t = &g->transition;

        switch (t->phase) {
        case TRANSITION_NONE: break;
        case TRANSITION_FADE_IN: {
                if (++t->ticks < TRANSITION_TICKS) break;
                t->phase          = TRANSITION_FADE_OUT;
                char filename[64] = {0};
                os_strcat(filename, ASSET_PATH_MAPS);
                os_strcat(filename, t->map);
                game_load_map(g, filename);
                obj_s *ohero;
                if (t->enterfrom) {
                        try_obj_from_handle(g->hero.obj, &ohero);
                        g->cam        = t->camprev;
                        v2_i32 offset = {g->cam.pos.x - t->heroprev.x,
                                         g->cam.pos.y - t->heroprev.y};
                        ohero->pos.x  = t->heroprev.x;
                        ohero->pos.y  = t->heroprev.y;

                        switch (t->enterfrom) {
                        case 0: break;
                        case DIRECTION_W:
                                ohero->pos.x = g->pixel_x - ohero->w - 4;
                                break;
                        case DIRECTION_E:
                                ohero->pos.x = 4;
                                break;
                        case DIRECTION_N:
                                ohero->pos.y = g->pixel_y - ohero->h - 4;
                                break;
                        case DIRECTION_S:
                                ohero->pos.y = 4;
                                break;
                        }
                        g->cam.pos = v2_add(ohero->pos, offset);
                        cam_constrain_to_room(g, &g->cam);
                }

        } break;
        case TRANSITION_FADE_OUT:
                if (--t->ticks == 0)
                        t->phase = TRANSITION_NONE;
                break;
        }
}