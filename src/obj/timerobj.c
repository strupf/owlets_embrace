// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32 trigger;
} timerobj_s;

obj_s *timerobj_create(g_s *g)
{
    obj_s      *o = obj_create(g);
    timerobj_s *t = (timerobj_s *)o->mem;
    o->ID         = OBJ_ID_TIMER;

    return o;
}

void timerobj_on_update(g_s *g, obj_s *o)
{
    timerobj_s *t = (timerobj_s *)o->mem;
    if (o->timer) {
        o->timer++;
        if (o->subtimer <= o->timer) {
            game_on_trigger(g, t->trigger);
            o->timer = 0;
        }
    }
}

bool32 timerobj_active(obj_s *o)
{
    return o->timer;
}

ratio_s timerobj_ratio(obj_s *o)
{
    ratio_s r = {o->timer, o->subtimer};
    return r;
}