// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void obj_custom_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    switch (o->ID) {
    default: break;
    case OBJ_ID_CRUMBLEBLOCK: crumbleblock_on_draw(g, o, cam); break;
    case OBJ_ID_TOGGLEBLOCK: toggleblock_on_draw(g, o, cam); break;
    case OBJ_ID_FALLINGBLOCK: fallingblock_on_draw(g, o, cam); break;
    case OBJ_ID_PUSHBLOCK: pushblock_on_draw(g, o, cam); break;
    }
}
