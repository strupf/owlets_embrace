// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
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
    case OBJ_ID_DOOR: door_on_draw(g, o, cam); break;
    case OBJ_ID_SPIDERBOSS: spiderboss_on_draw(g, o, cam); break;
    case OBJ_ID_TRAMPOLINE: trampoline_on_draw(g, o, cam); break;
    case OBJ_ID_SAVEPOINT: savepoint_on_draw(g, o, cam); break;
    case OBJ_ID_BITER: biter_on_draw(g, o, cam); break;
    }
}
