// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void obj_custom_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    switch (o->ID) {
    default: break;
    case OBJID_CRUMBLEBLOCK: crumbleblock_on_draw(g, o, cam); break;
    case OBJID_TOGGLEBLOCK: toggleblock_on_draw(g, o, cam); break;
    case OBJID_FALLINGBLOCK: fallingblock_on_draw(g, o, cam); break;
    case OBJID_PUSHBLOCK: pushblock_on_draw(g, o, cam); break;
    case OBJID_DOOR: door_on_draw(g, o, cam); break;
    case OBJID_SPIDERBOSS: spiderboss_on_draw(g, o, cam); break;
    case OBJID_TRAMPOLINE: trampoline_on_draw(g, o, cam); break;
    case OBJID_SAVEPOINT: savepoint_on_draw(g, o, cam); break;
    case OBJID_BITER: biter_on_draw(g, o, cam); break;
    case OBJID_WATERCOL: watercol_on_draw(g, o, cam); break;
    }
}
