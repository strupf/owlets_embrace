/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#include "hero.h"
#include "game.h"

enum hero_const {
        HERO_C_JUMP_INIT = -300,
        HERO_C_ACCX      = 20,
        HERO_C_JUMP_MAX  = 60,
        HERO_C_JUMP_MIN  = 0,
        HERO_C_JUMPTICKS = 20,
        HERO_C_EDGETICKS = 8,
        HERO_C_GRAVITY   = 32,
};

obj_s *hero_create(game_s *g, hero_s *h)
{
        obj_s *hero = obj_create(g);

        hero->flags = objflags_create(OBJ_FLAG_ACTOR,
                                      OBJ_FLAG_HERO);
        hero->actorflags |= ACTOR_FLAG_CLIMB_SLOPES;
        hero->pos.x        = 10;
        hero->pos.y        = 20;
        hero->w            = 16;
        hero->h            = 24;
        hero->gravity_q8.y = HERO_C_GRAVITY;
        hero->drag_q8.x    = 256;
        hero->drag_q8.y    = 256; // no drag
        *h                 = (const hero_s){0};
        h->obj             = objhandle_from_obj(hero);
        return hero;
}

void hero_update(game_s *g, obj_s *o, hero_s *h)
{
#if 1
        bool32 grounded = game_area_blocked(g, obj_rec_bottom(o));
        if (grounded) {
                h->edgeticks = HERO_C_EDGETICKS;
        } else if (h->edgeticks > 0) {
                h->edgeticks--;
        }

        if (h->inp & HERO_INP_JUMP) {
                if (!(h->inpp & HERO_INP_JUMP) && h->edgeticks > 0) {
                        h->edgeticks = 0;
                        h->jumpticks = HERO_C_JUMPTICKS;
                        o->vel_q8.y  = HERO_C_JUMP_INIT;
                } else if (h->jumpticks > 0) {
                        i32 jt = HERO_C_JUMPTICKS - h->jumpticks;
                        int j  = HERO_C_JUMP_MAX -
                                ease_out_q(HERO_C_JUMP_MIN,
                                           HERO_C_JUMP_MAX,
                                           HERO_C_JUMPTICKS,
                                           jt, 2);
                        o->vel_q8.y -= j;
                        h->jumpticks--;
                }
        } else {
                h->jumpticks = 0;
        }

        int xs = 0;
        if (h->inp & HERO_INP_LEFT) xs = -1;
        if (h->inp & HERO_INP_RIGHT) xs = +1;

        o->vel_q8.x += xs * HERO_C_ACCX;

        if (xs == 0 && grounded) {
                o->drag_q8.x = 180;
        } else {
                o->drag_q8.x = 256;
        }
#else
        if (debug_inp_up()) {
                obj_move_y(g, o, -1);
        }
        if (debug_inp_down()) {
                obj_move_y(g, o, +1);
        }
        if (debug_inp_left()) {
                obj_move_x(g, o, -1);
        }
        if (debug_inp_right()) {
                obj_move_x(g, o, +1);
        }
#endif
}