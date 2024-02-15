// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *shortcut_create(game_s *g)
{
    obj_s *o = obj_create(g);

    o->flags = OBJ_FLAG_RENDER_AABB;

    o->w = 32;
    o->h = 32;

    return o;
}

void shortcut_load(game_s *g, map_obj_s *mo)
{
    obj_s *o = shortcut_create(g);
    o->pos.x = mo->x;
    o->pos.y = mo->y;

    o->substate = map_obj_i32(mo, "ID");
}