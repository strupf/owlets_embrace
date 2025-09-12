// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    CAMATTRACTOR_PT,
    CAMATTRACTOR_LINE
};

typedef struct {
    i32    r;
    i32    trigger_on;
    i32    trigger_off;
    i32    n_arr;
    v2_i16 arr[8];
} camattractor_s;

void camattractor_on_trigger(g_s *g, obj_s *o, i32 trigger);

void camattractor_load(g_s *g, map_obj_s *mo)
{
    i32 i = map_obj_i32(mo, "only_if_not_saveID");
    if (i && save_event_exists(g, i)) return;

    obj_s          *o = obj_create(g);
    camattractor_s *c = (camattractor_s *)o->mem;
    o->UUID           = mo->UUID;
    o->ID             = OBJID_CAMATTRACTOR;
    o->pos.x          = mo->x + (mo->w >> 1);
    o->pos.y          = mo->y + (mo->h >> 1);
    c->r              = map_obj_i32(mo, "r");
    o->substate       = CAMATTRACTOR_PT;

    if (map_obj_bool(mo, "active")) {
        o->cam_attract_r = c->r;
    }

    if (map_obj_has_nonnull_prop(mo, "pt")) {
        o->substate        = CAMATTRACTOR_LINE;
        v2_i16 pt          = map_obj_pt(mo, "pt");
        c->arr[c->n_arr++] = v2_i16_shl(pt, 4);
    }

    c->trigger_on  = map_obj_i32(mo, "trigger_on");
    c->trigger_off = map_obj_i32(mo, "trigger_off");
    o->on_trigger  = camattractor_on_trigger;
}

v2_i32 camattractor_closest_pt(obj_s *o, v2_i32 pt)
{
    switch (o->substate) {
    case CAMATTRACTOR_PT: {
        return o->pos;
    }
    case CAMATTRACTOR_LINE: {
        camattractor_s *c = (camattractor_s *)o->mem;
        v2_i32          p = v2_i32_from_i16(c->arr[0]);
        return project_pnt_line(pt, o->pos, p);
    }
    }

    return o->pos;
}

void camattractor_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    camattractor_s *c = (camattractor_s *)o->mem;

    if (0) {
    } else if (trigger == c->trigger_on) {
        o->cam_attract_r = c->r;
    } else if (trigger == c->trigger_off) {
        o->cam_attract_r = 0;
    }
}