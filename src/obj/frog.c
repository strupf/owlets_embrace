// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32 x;
} frog_s;

void frog_on_update(game_s *g, obj_s *o);
void frog_on_animate(game_s *g, obj_s *o);

void frog_load(game_s *g, map_obj_s *mo)
{
    obj_s  *o     = obj_create(g);
    frog_s *f     = (frog_s *)o->mem;
    o->ID         = OBJ_ID_FROG;
    o->on_animate = frog_on_animate;
    o->on_update  = frog_on_update;
    o->w          = 16;
    o->h          = 16;
    o->grav_q8.y  = 80;
    o->flags =
        OBJ_FLAG_ENEMY |
        OBJ_FLAG_MOVER;
}

void frog_on_update(game_s *g, obj_s *o)
{
    frog_s *f = (frog_s *)o->mem;
}

void frog_on_animate(game_s *g, obj_s *o)
{
    frog_s *f = (frog_s *)o->mem;
}