// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

static void walker_find_new_edge(game_s *g, obj_s *o)
{
}

void walker_on_update(game_s *g, obj_s *o)
{
    const rec_i32 aabb = obj_aabb(o);

    switch (o->state) {
    case 0: // glued to right
        break;
    case 1: // glued to up
        break;
    case 2: // glued to down
        break;
    case 3: // glued to left
        break;
    }
}