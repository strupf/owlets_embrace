// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#if 0
typedef struct {
    obj_s o;
} blob_s;

void blob_update(game_s *g, obj_s *obj);
void blob_animate(game_s *g, obj_s *obj);

obj_s *blob_create(game_s *g)
{
    blob_s *o = (blob_s *)obj_create(g);
    o->o.ID   = OBJ_ID_BLOB;
    o->o.flags |= OBJ_FLAG_ACTOR;
    o->o.flags |= OBJ_FLAG_MOVER;
    o->o.w            = 16;
    o->o.h            = 16;
    o->o.gravity_q8.y = 30;
    o->o.on_update    = blob_update;
    o->o.on_animate   = blob_animate;

    return (obj_s *)o;
}

void blob_update(game_s *g, obj_s *obj)
{
    blob_s *o = (blob_s *)obj;
}

void blob_animate(game_s *g, obj_s *obj)
{
    blob_s *o = (blob_s *)obj;
}
#endif