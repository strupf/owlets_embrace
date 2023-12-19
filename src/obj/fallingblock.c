// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *fallingblock_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_FALLINGBLOCK;
    o->flags |= OBJ_FLAG_SOLID;
    o->w = 16;
    o->h = 16;
    return o;
}

void fallingblock_on_update(game_s *g, obj_s *o)
{
}

void fallingblock_on_animate(game_s *g, obj_s *o)
{
}