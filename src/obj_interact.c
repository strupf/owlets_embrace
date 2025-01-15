// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void obj_interact(g_s *g, obj_s *o, obj_s *ohero)
{
    if (!o) return;

    switch (o->ID) {
    default: break;
    case OBJID_NPC: npc_on_interact(g, o); break;
    }
}