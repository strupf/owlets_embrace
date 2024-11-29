// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void walker_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_WALKER;
    o->flags = OBJ_FLAG_MOVER |
               OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_HERO_STOMPABLE |
               OBJ_FLAG_KILL_OFFSCREEN;
    o->moverflags = OBJ_MOVER_MAP | OBJ_MOVER_ONE_WAY_PLAT;
    o->w          = 20;
    o->h          = 30;
    o->pos.x      = mo->x;
    o->pos.y      = mo->y + mo->h - o->h;

    o->health_max = 3;
    o->health     = o->health_max;
    o->enemy      = enemy_default();
    o->facing     = 1;
    o->n_sprites  = 0;
}

void walker_on_update(g_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q8.y = 0;
    }

    if ((o->bumpflags & OBJ_BUMP_X) || obj_would_fall_down_next(g, o, o->facing)) {
        o->facing = -o->facing;
    }

    o->bumpflags = 0;

    o->timer++;
    if (o->timer & 1) {
        obj_move(g, o, o->facing, 0);
    }

    o->v_q8.y += 60;
    obj_move_by_v_q8(g, o);
}

void walker_on_animate(g_s *g, obj_s *o)
{
}