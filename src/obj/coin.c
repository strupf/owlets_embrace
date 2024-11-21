// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define COIN_TIME 200

obj_s *coin_create(g_s *g)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJ_ID_COIN;
    o->w          = 16;
    o->h          = 16;
    o->flags      = OBJ_FLAG_RENDER_AABB;
    o->moverflags = OBJ_MOVER_MAP | OBJ_MOVER_ONE_WAY_PLAT;
    o->timer      = 200;
    return o;
}

void coin_on_animate(g_s *g, obj_s *o)
{
}

void coin_on_update(g_s *g, obj_s *o)
{
    if (--o->timer <= 0) {
        obj_delete(g, o);
        return;
    }

    o->v_q8.y += 60;
    obj_move_by_v_q8(g, o);

    if (obj_grounded(g, o)) {
        obj_vx_q8_mul(o, 230);
    }

    if (o->bumpflags & OBJ_BUMP_X) {
        obj_vx_q8_mul(o, -192);
    }
    if (o->bumpflags & OBJ_BUMP_Y) {
        if (o->v_q8.y < 400) {
            o->v_q8.y = 0;
        } else {
            obj_vy_q8_mul(o, -192);
        }
    }
    o->bumpflags = 0;

    if (abs_i32(o->v_q8.x) < 16) {
        o->v_q8.x = 0;
    }
    if (abs_i32(o->v_q8.y) < 16) {
        o->v_q8.y = 0;
    }
}