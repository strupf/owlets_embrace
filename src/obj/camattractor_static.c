// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    CAMATTRACTOR_PT,
    CAMATTRACTOR_LINE
};

void camattractor_static_load(game_s *g, map_obj_s *mo)
{
    obj_s *o         = obj_create(g);
    o->ID            = OBJ_ID_CAMATTRACTOR;
    o->pos.x         = mo->x;
    o->pos.y         = mo->y;
    o->cam_attract_r = 300;
    o->state         = CAMATTRACTOR_PT;

    if (map_obj_has_nonnull_prop(mo, "Pt")) {
        o->state          = CAMATTRACTOR_LINE;
        v2_i32 pt         = v2_i32_from_i16(map_obj_pt(mo, "Pt"));
        *(v2_i32 *)o->mem = v2_shl(pt, 4);
    }
}

v2_i32 camattractor_static_closest_pt(obj_s *o, v2_i32 pt)
{
    if (o->state == CAMATTRACTOR_PT)
        return o->pos;
    return project_pnt_line(pt, o->pos, *(v2_i32 *)o->mem);
}