// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *walker_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_WALKER;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
#if GAME_JUMP_ATTACK
               OBJ_FLAG_PLATFORM | OBJ_FLAG_PLATFORM_HERO_ONLY |
#endif
               OBJ_FLAG_KILL_OFFSCREEN;

    o->w            = 16;
    o->h            = 16;
    o->health_max   = 1;
    o->health       = o->health_max;
    o->enemy        = enemy_default();
    o->gravity_q8.y = 50;
    o->facing       = 1;
    return o;
}

void walker_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = walker_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y;
}

void walker_on_update(game_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = 0;
    }

    if ((o->bumpflags & OBJ_BUMPED_X) || obj_would_fall_down_next(g, o, o->facing)) {
        o->facing = -o->facing;
    }
    o->bumpflags = 0;
    o->tomove.x  = o->facing;
}