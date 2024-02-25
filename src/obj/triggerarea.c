// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void triggerarea_load(game_s *g, map_obj_s *mo)
{
    obj_s *o   = obj_create(g);
    o->ID      = OBJ_ID_TRIGGERAREA;
    o->pos.x   = mo->x;
    o->pos.y   = mo->y;
    o->w       = mo->w;
    o->h       = mo->h;
    o->trigger = map_obj_i32(mo, "trigger");
    o->state   = map_obj_bool(mo, "once");
}

void triggerarea_update(game_s *g, obj_s *o)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!ohero) return;

    if (overlap_rec(obj_aabb(ohero), obj_aabb(o))) {
        game_on_trigger(g, o->trigger);
        if (o->state) { // only once
            obj_delete(g, o);
        }
    }
}