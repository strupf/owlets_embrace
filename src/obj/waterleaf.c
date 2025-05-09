// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    WATERLEAF_IDLE,
    WATERLEAF_SHAKING,
    WATERLEAF_SINKING,
    WATERLEAF_RISING,
};

typedef struct {
    v2_i32 p_og;
} waterleaf_s;

void waterleaf_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJID_WATERLEAF;
    o->flags = OBJ_FLAG_PLATFORM |
               OBJ_FLAG_RENDER_AABB;
    o->pos.x       = mo->x;
    o->pos.y       = mo->y;
    o->w           = mo->w;
    o->h           = 16;
    waterleaf_s *w = (waterleaf_s *)o->mem;
    w->p_og        = o->pos;
}

void waterleaf_on_update(g_s *g, obj_s *o)
{
    rec_i32 r  = obj_aabb(o);
    i32     wd = water_depth_rec(g, r);

    bool32 sunk  = 0;
    obj_s *ohero = obj_get_hero(g);
    if (ohero) {
        rec_i32 r1 = {o->pos.x, o->pos.y, o->w, 1};
        rec_i32 r2 = obj_rec_bottom(ohero);
        if (overlap_rec(r1, r2)) {
            obj_move(g, o, 0, +1);
            sunk = 1;
        }
    }

    switch (o->state) {
    case WATERLEAF_IDLE: {
        break;
    }
    }

    if (!sunk && o->h <= wd) {
        obj_move(g, o, 0, -1);
    }
}

void waterleaf_on_animate(g_s *g, obj_s *o)
{
}