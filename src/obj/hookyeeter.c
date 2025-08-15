// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    HOOKYEETER_ST_IDLE,
    HOOKYEETER_ST_HOOKED,
};

obj_s *hookyeeter_create(g_s *g)
{

    obj_s *o = obj_create(g);
    o->ID    = OBJID_HOOKYEETER;
    o->w     = 32;
    o->h     = 32;
    o->flags = OBJ_FLAG_HOOKABLE;
    return o;
}

void hookyeeter_on_update(g_s *g, obj_s *o)
{
    if (o->state == HOOKYEETER_ST_HOOKED) {
        grapplinghook_s *gh = &g->ghook;
        o->timer++;
        if (o->timer == 10) {
            hookyeeter_on_unhook(g, o);
        } else {
            gh->len_max_q4 = (gh->len_max_q4 * 236) >> 8;
        }
    }
}

void hookyeeter_on_hook(g_s *g, obj_s *o)
{
    o->state = HOOKYEETER_ST_HOOKED;
    o->timer = 0;
}

void hookyeeter_on_unhook(g_s *g, obj_s *o)
{
    o->state = HOOKYEETER_ST_IDLE;
    o->timer = 0;
    HOOKYEETER_ST_IDLE;
    grapplinghook_destroy(g, &g->ghook);
}