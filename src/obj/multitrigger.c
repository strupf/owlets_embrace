// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// splits a single trigger into multiple triggers

#include "game.h"

#define MULTITRIGGER_N 16

typedef struct {
    b32 once;
    i32 trigger;
    i32 trigger_list[MULTITRIGGER_N];
} multitrigger_s;

void multitrigger_on_trigger(g_s *g, obj_s *o, i32 trigger);

void multitrigger_load(g_s *g, map_obj_s *mo)
{
    if (!map_obj_check_spawn_saveIDs(g, mo)) return;

    obj_s          *o = obj_create(g);
    multitrigger_s *m = (multitrigger_s *)o->mem;
    o->ID             = OBJID_MULTITRIGGER;
    o->editorUID      = mo->UID;
    o->on_trigger     = multitrigger_on_trigger;
    m->trigger        = map_obj_i32(mo, "trigger");
    m->once           = map_obj_bool(mo, "once");

    i32  n_trigger = 0;
    i32 *t         = (i32 *)map_obj_arr(mo, "triggers", &n_trigger);

    assert(n_trigger <= MULTITRIGGER_N);
    for (i32 n = 0; n < n_trigger; n++) {
        assert(t[n] != m->trigger);
        m->trigger_list[n] = t[n];
    }
}

void multitrigger_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    multitrigger_s *m = (multitrigger_s *)o->mem;
    if (trigger != m->trigger) return;

    for (i32 n = 0; n < MULTITRIGGER_N; n++) {
        game_on_trigger(g, m->trigger_list[n]);
    }

    if (m->once) {
        obj_delete(g, o);
    }
}
