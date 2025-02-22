// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    bool32 occupied;
    i32    once;
    i32    trigger_enter;
    i32    trigger_leave;
    i32    saveID;
} triggerarea_s;

void triggerarea_on_update(g_s *g, obj_s *o);

obj_s *triggerarea_spawn(g_s *g, rec_i32 r, i32 tr_enter, i32 tr_leave, b32 once)
{
    obj_s         *o = obj_create(g);
    triggerarea_s *t = (triggerarea_s *)&o->mem;
    o->ID            = OBJID_TRIGGERAREA;
    o->on_update     = triggerarea_on_update;
    o->pos.x         = r.x;
    o->pos.y         = r.y;
    o->w             = r.w;
    o->h             = r.h;
    t->trigger_enter = tr_enter;
    t->trigger_leave = tr_leave;
    t->once          = once;
    return o;
}

void triggerarea_load(g_s *g, map_obj_s *mo)
{
    i32 saveID = map_obj_i32(mo, "saveID");
    if (save_event_exists(g, saveID)) return;

    rec_i32        r = {mo->x, mo->y, mo->w, mo->h};
    obj_s         *o = triggerarea_spawn(g, r,
                                         map_obj_i32(mo, "trigger_enter"),
                                         map_obj_i32(mo, "trigger_leave"),
                                         map_obj_bool(mo, "once"));
    triggerarea_s *t = (triggerarea_s *)&o->mem;
    t->saveID        = saveID;
}

void triggerarea_on_update(g_s *g, obj_s *o)
{
    triggerarea_s *t     = (triggerarea_s *)&o->mem;
    obj_s         *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!ohero) return;

    bool32 occupied = overlap_rec(obj_aabb(ohero), obj_aabb(o));
    if (occupied && !t->occupied && t->trigger_enter) {
        game_on_trigger(g, t->trigger_enter);
        if (t->once) { // only once
            save_event_register(g, t->saveID);
            obj_delete(g, o);
        }
    }
    if (!occupied && t->occupied && t->trigger_leave) {
        game_on_trigger(g, t->trigger_leave);
        if (t->once) { // only once
            save_event_register(g, t->saveID);
            obj_delete(g, o);
        }
    }
    t->occupied = occupied;
}