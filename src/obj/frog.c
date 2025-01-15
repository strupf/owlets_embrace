// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    FROG_STATE_SLURP,
    FROG_STATE_,
};

typedef struct {
    i32 x;
} frog_s;

void frog_load(g_s *g, map_obj_s *mo)
{
    obj_s  *o     = obj_create(g);
    frog_s *f     = (frog_s *)o->mem;
    o->ID         = OBJID_FROG;
    o->on_animate = frog_on_animate;
    o->on_update  = frog_on_update;
    o->w          = 16;
    o->h          = 16;
    o->flags =
        OBJ_FLAG_ENEMY |
        OBJ_FLAG_MOVER;
}

void frog_on_update(g_s *g, obj_s *o)
{
    frog_s *f = (frog_s *)o->mem;
}

void frog_on_animate(g_s *g, obj_s *o)
{
    frog_s *f = (frog_s *)o->mem;
}