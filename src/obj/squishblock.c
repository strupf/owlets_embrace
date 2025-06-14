// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 anchor;
} squishblock_s;

void squishblock_on_update(g_s *g, obj_s *o);
void squishblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void squishblock_create(g_s *g, map_obj_s *mo)
{
    obj_s *o     = obj_create(g);
    o->on_update = squishblock_on_update;
    o->on_draw   = squishblock_on_draw;
    o->pos.x     = mo->x;
    o->pos.y     = mo->y;
    o->w         = mo->w;
    o->h         = mo->h;
    o->flags     = OBJ_FLAG_SOLID;
}

void squishblock_on_update(g_s *g, obj_s *o)
{
}

void squishblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    i32 x_total  = 50;
    i32 w_attach = 50;
    i32 w_mid    = 20;
    i32 x_half   = x_total >> 1;
    i32 a_q16    = ((w_attach - w_mid) << 16) / pow_i32(x_half, 2);

    for (i32 x = 0; x < x_half; x++) {

        i32 w = w_mid + ((a_q16 * pow_i32(-(x - x_half), 2)) >> 16);
    }
}