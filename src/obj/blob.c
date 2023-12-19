// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *blob_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_BLOB;
    o->flags |= OBJ_FLAG_ACTOR;
    o->flags |= OBJ_FLAG_MOVER;
    o->w            = 16;
    o->h            = 16;
    o->gravity_q8.y = 30;
    o->drag_q8.x    = 0;
    o->drag_q8.y    = 256;
    return o;
}

void blob_on_update(game_s *g, obj_s *o)
{
    bool32 grounded = obj_grounded(g, o);
    if (grounded) {
        o->drag_q8.x = 0;
        if (--o->timer > 0) return;
        o->vel_q8.y = -3000;
        o->vel_q8.x = rngr_i32(-1000, +1000);
        o->timer    = 100;
    } else {
        o->drag_q8.x = 255;
        o->timer     = 100;
    }
}

void blob_on_animate(game_s *g, obj_s *o)
{
    bool32 grounded = obj_grounded(g, o);
}