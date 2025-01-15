// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void objs_trigger(g_s *g, i32 trigger)
{
    for (obj_each(g, o)) {
        switch (o->ID) {
        default: break;
        case OBJID_TOGGLEBLOCK: toggleblock_on_trigger(g, o, trigger); break;
        case OBJID_DOOR: door_on_trigger(g, o, trigger); break;
        case OBJID_CLOCKPULSE: clockpulse_on_trigger(g, o, trigger); break;
        }
    }
}