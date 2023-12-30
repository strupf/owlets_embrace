// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *crawler_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_CRAWLER;
    return o;
}

static void crawler_find_new_edge(game_s *g, obj_s *o)
{
}

void crawler_on_update(game_s *g, obj_s *o)
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

void crawler_on_animate(game_s *g, obj_s *o)
{
}