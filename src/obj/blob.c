// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define BLOB_DRAG_AIR 255

obj_s *blob_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_BLOB;
    o->flags |= OBJ_FLAG_ACTOR;
    o->flags |= OBJ_FLAG_MOVER;
    o->flags |= OBJ_FLAG_KILL_OFFSCREEN;
    o->w            = 16;
    o->h            = 16;
    o->gravity_q8.y = 35;
    o->drag_q8.x    = 0;
    o->drag_q8.y    = 256;
    return o;
}

void blob_on_update(game_s *g, obj_s *o)
{
    if (o->bumpflags & OBJ_BUMPED_X)
        o->vel_q8.x = -o->vel_q8.x >> 1;
    if (o->bumpflags & OBJ_BUMPED_Y)
        o->vel_q8.y = 0;
    o->bumpflags = 0;

    bool32 in_air = !obj_grounded(g, o);

    o->drag_q8.x = in_air ? BLOB_DRAG_AIR : 0;
    if (in_air) {
        o->timer = rngr_i32(30, 80);
    } else if (--o->timer <= 0) {
        o->drag_q8.x = BLOB_DRAG_AIR;
        o->vel_q8.y  = -rngr_i32(600, 1200);
        o->vel_q8.x  = rngr_i32(-1000, +1000);
    }
}

void blob_on_animate(game_s *g, obj_s *o)
{
    bool32 grounded = obj_grounded(g, o);
    if (grounded) {
        if (20 < o->timer) {
            // jump anticipation wiggle
        } else {
        }
    } else { // in air
        if (0 < o->vel_q8.y) {

        } else {
        }
    }
}

void blob_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    rec_i32   r   = obj_aabb(o);
    r             = translate_rec(r, cam);
    gfx_rec_fill(ctx, r, PRIM_MODE_BLACK);
}