/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#include "hero.h"
#include "game.h"

enum hero_const {
        HERO_C_JUMP_INIT = -1000,
        HERO_C_ACCX      = 100,
        HERO_C_JUMP_MAX  = 100,
        HERO_C_JUMP_MIN  = 10,
        HERO_C_JUMPTICKS = 10,
        HERO_C_EDGETICKS = 8,

};

obj_s *hero_create(game_s *g, hero_s *h)
{
        obj_s *hero        = obj_create(g);
        hero->flags        = objflags_create(OBJ_FLAG_ACTOR,
                                             OBJ_FLAG_HERO);
        hero->actorflags |= ACTOR_FLAG_CLIMB_SLOPES;
        hero->pos.x        = 10;
        hero->pos.y        = 20;
        hero->w            = 16;
        hero->h            = 32;
        hero->gravity_q8.y = 20;
        hero->drag_q8.x    = 200;
        hero->drag_q8.y    = 256; // no drag
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

        if ((h->inp & HERO_INP_JUMP) &&
            o->vel_q8.y >= 0 &&
            grounded) {
                o->vel_q8.y = HERO_C_JUMP_INIT;
        }

        int xs = 0;
        if (h->inp & HERO_INP_LEFT) xs = -1;
        if (h->inp & HERO_INP_RIGHT) xs = +1;

        o->vel_q8.x += xs * HERO_C_ACCX;
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