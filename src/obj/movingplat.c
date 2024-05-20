// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    MOVPL_TYPE_PINGPONG,
    MOVPL_TYPE_CIRCLE,
};

enum {
    MOVPL_CONTINUOUS,
    MOVPL_STOP_AT_END,
    MOVPL_STOP_AT_EACH
};

typedef struct {
    i32    i0;
    i32    i1;
    i32    p_q4;
    i32    v_q4;
    i32    v_trg_q4;
    i32    n_path;
    v2_i16 path[16];
} movingplatform_s;

void movingplatform_on_update(game_s *g, obj_s *o)
{
    movingplatform_s *plat = (movingplatform_s *)o->mem;
    if (plat->v_q4 == 0 || plat->n_path < 2) return;
    assert(0 < plat->v_q4);
    plat->p_q4 += plat->v_q4;

    // while -> allows for very fast movements, possibly skipping
    // vertices along the path in one frame
    while (1) {
        v2_i32 p0_q4  = v2_shl(v2_i32_from_i16(plat->path[plat->i0]), 4);
        v2_i32 p1_q4  = v2_shl(v2_i32_from_i16(plat->path[plat->i1]), 4);
        i32    distsq = v2_distancesq(p0_q4, p1_q4);

        if (pow2_i32(plat->p_q4) < distsq) break;

        // next vertex
        plat->p_q4 -= sqrt_i32(distsq);

        // determine if moving in ascending or descending
        // through the position array
        bool32 dir_pos = (plat->i0 + 1) % plat->n_path == plat->i1;
        plat->i0       = plat->i1;

        // update from and to vertex
        if (dir_pos) {
            plat->i1++;
            if (plat->n_path <= plat->i1) {
                switch (o->state) {
                case MOVPL_TYPE_PINGPONG: plat->i1 = plat->n_path - 2; break;
                case MOVPL_TYPE_CIRCLE: plat->i1 = 0; break;
                }
            }
        } else {
            plat->i1--;
            if (plat->i1 < 0) {
                switch (o->state) {
                case MOVPL_TYPE_PINGPONG: plat->i1 = 1; break;
                case MOVPL_TYPE_CIRCLE: plat->i1 = plat->n_path - 1; break;
                }
            }
        }

        // determine stops
        switch (o->substate) {
        case MOVPL_STOP_AT_END: {
            bool32 stop_at = 0;
            switch (o->state) {
            case MOVPL_TYPE_PINGPONG:
                if (plat->i0 == 0 || plat->i0 == plat->n_path - 1)
                    stop_at = 1;
                break;
            case MOVPL_TYPE_CIRCLE:
                if (plat->i0 == 0)
                    stop_at = 1;
                break;
            }

            if (!stop_at) break;
            // fallthrough
        }
        case MOVPL_STOP_AT_EACH:
            plat->p_q4 = 0;
            plat->v_q4 = 0;
            break;
        }
    }

    v2_i32 p0  = v2_i32_from_i16(plat->path[plat->i0]);
    v2_i32 p1  = v2_i32_from_i16(plat->path[plat->i1]);
    i32    num = plat->p_q4 >> 4;
    i32    den = v2_distance(p0, p1);
    v2_i32 pi  = {p0.x + ((p1.x - p0.x) * num) / den,
                  p0.y + ((p1.y - p0.y) * num) / den};
    o->tomove  = v2_sub(pi, o->pos);
}

void movingplatform_on_trigger(game_s *g, obj_s *o, i32 trigger)
{
    if (o->trigger != trigger) return;
    movingplatform_s *plat = (movingplatform_s *)o->mem;
    if (plat->v_q4) return;
    plat->v_q4 = plat->v_trg_q4;
}

void movingplatform_load(game_s *g, map_obj_s *mo)
{
    obj_s            *o    = obj_create(g);
    movingplatform_s *plat = (movingplatform_s *)o->mem;
    o->ID                  = OBJ_ID_MOVINGPLATFORM;
    o->flags               = OBJ_FLAG_RENDER_AABB;
    o->on_trigger          = movingplatform_on_trigger;
    if (map_obj_bool(mo, "Solid")) {
        o->flags |= OBJ_FLAG_SOLID;
    } else {
        o->flags |= OBJ_FLAG_PLATFORM;
    }
    o->on_update   = movingplatform_on_update;
    o->w           = 64;
    o->h           = 16;
    o->pos.x       = mo->x;
    o->pos.y       = mo->y;
    o->w           = mo->w;
    o->h           = mo->h;
    o->trigger     = map_obj_i32(mo, "Trigger_Move");
    plat->i0       = 0;
    plat->i1       = 1;
    plat->v_trg_q4 = map_obj_i32(mo, "V");

    if (!o->trigger) {
        plat->v_q4 = plat->v_trg_q4;
    }

    i32     num_lin = 0;
    i32     num_cir = 0;
    v2_i16 *pts_lin = (v2_i16 *)map_obj_arr(mo, "Path_Linear", &num_lin);
    v2_i16 *pts_cir = (v2_i16 *)map_obj_arr(mo, "Path_Circular", &num_cir);
    v2_i16 *pts     = NULL;
    i32     num     = 0;

    o->substate = map_obj_i32(mo, "Moving_Platform_Stop");

    if (pts_lin && num_lin) {
        pts      = pts_lin;
        num      = num_lin;
        o->state = MOVPL_TYPE_PINGPONG;
    }
    if (pts_cir && num_cir) {
        pts      = pts_cir;
        num      = num_cir;
        o->state = MOVPL_TYPE_CIRCLE;
    }

    plat->path[0].x = o->pos.x;
    plat->path[0].y = o->pos.y;
    plat->n_path    = 1;
    if (pts && num) {
        for (int n = 0; n < num; n++) {
            v2_i16 v = pts[n];
            v.x <<= 4;
            v.y <<= 4;
            plat->path[plat->n_path++] = v;
        }
    }
}
