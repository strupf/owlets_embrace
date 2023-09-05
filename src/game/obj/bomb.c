// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/game.h"

static void bomb_explode(game_s *g, obj_s *o, void *arg)
{
        obj_delete(g, o);
}

void bomb_think(game_s *g, obj_s *o, void *arg)
{
        if (--o->timer == 0) {
                bomb_explode(g, o, arg);
        }
}

obj_s *bomb_create(game_s *g, v2_i32 p, v2_i32 v_q8)
{
        obj_s     *bomb  = obj_create(g);
        objflags_s flags = objflags_create(
            OBJ_FLAG_ACTOR,
            OBJ_FLAG_MOVABLE,
            OBJ_FLAG_THINK_1,
            OBJ_FLAG_KILL_OFFSCREEN);
        obj_apply_flags(g, bomb, flags);
        bomb->think_1      = bomb_think;
        bomb->w            = 8;
        bomb->h            = 8;
        bomb->pos.x        = p.x - bomb->w / 2;
        bomb->pos.y        = p.y - bomb->h / 2;
        bomb->vel_q8       = v_q8;
        bomb->gravity_q8.y = 12;
        bomb->drag_q8.x    = 256;
        bomb->drag_q8.y    = 256;
        bomb->onsqueeze    = bomb_explode;
        return bomb;
}