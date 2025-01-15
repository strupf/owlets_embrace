// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    CAMATTRACTOR_PT,
    CAMATTRACTOR_LINE
};

typedef struct {
    i32    n_arr;
    v2_i16 arr[8];
} camattractor_s;

void camattractor_static_load(g_s *g, map_obj_s *mo)
{
    obj_s          *o = obj_create(g);
    camattractor_s *c = (camattractor_s *)o->mem;
    o->ID             = OBJID_CAMATTRACTOR;
    o->pos.x          = mo->x;
    o->pos.y          = mo->y;
    o->cam_attract_r  = 300;
    o->state          = CAMATTRACTOR_PT;

    if (map_obj_has_nonnull_prop(mo, "Pt")) {
        o->state           = CAMATTRACTOR_LINE;
        v2_i16 pt          = map_obj_pt(mo, "Pt");
        c->arr[c->n_arr++] = v2_i16_shl(pt, 4);
    }
}

v2_i32 camattractor_static_closest_pt(obj_s *o, v2_i32 pt)
{
    switch (o->state) {
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