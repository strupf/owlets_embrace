// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    POT_CARRIED = 1,
    POT_THROWN,
};

void pot_on_update(game_s *g, obj_s *o)
{
    bool32 burst = 0;
    if (o->bumpflags & OBJ_BUMPED_Y) {
        if (500 <= abs_i32(o->vel_q8.y)) {
            burst = 1;
        }
        o->vel_q8.y = 0;
    }
    if (o->bumpflags & OBJ_BUMPED_X) {
        if (500 <= abs_i32(o->vel_q8.x)) {
            burst = 1;
        }
        o->vel_q8.x = 0;
    }
    o->bumpflags = 0;

    if (burst) {
        sys_printf("burst!\n");
    }
}

void pot_on_animate(game_s *g, obj_s *o)
{
}

void pot_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_POT;
    o->flags = OBJ_FLAG_SOLID |
               OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_RENDER_AABB;
    o->on_animate   = pot_on_animate;
    o->on_update    = pot_on_update;
    o->w            = 16;
    o->h            = 16;
    o->pos.x        = mo->x;
    o->pos.y        = mo->y;
    o->gravity_q8.y = 50;
    o->drag_q8.x    = 254;
    o->drag_q8.y    = 255;
}

void pot_on_pickup(game_s *g, obj_s *o, obj_s *ohero)
{
    o->flags &= ~OBJ_FLAG_MOVER;
    o->state = POT_CARRIED;
}

void pot_on_throw(game_s *g, obj_s *o, i32 dir_x)
{
    o->state = POT_THROWN;
    o->flags |= OBJ_FLAG_MOVER;
}