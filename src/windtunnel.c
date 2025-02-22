// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "windtunnel.h"
#include "game.h"

static v2_i32 windtunnel_decode(i32 t);
static i32    windtunnel_encode(i32 x, i32 y);

void windtunnel_init(g_s *g)
{
    windtunnel_s *t = 0;
    t->v            = (u8 *)game_alloc(g, g->tiles_x * g->tiles_y, 1);

    spm_push();
    spm_pop();
}

void windtunnel_update(g_s *g)
{
    windtunnel_s *t = 0;
}

static v2_i32 windtunnel_decode(i32 t)
{
    v2_i32 r = {0};
    r.x      = (t >> 4) & 3;
    if (t & 0x80) {
        r.x = -r.x;
    }
    r.y = (t) & 3;
    if (t & 0x08) {
        r.y = -r.y;
    }
    return r;
}

static i32 windtunnel_encode(i32 x, i32 y)
{
    i32 r = 0;
    if (0 <= x) {
        r |= ((+x) << 4);
    } else {
        r |= ((-x) << 4) | 0x80;
    }
    if (0 <= y) {
        r |= (+y);
    } else {
        r |= (-y) | 0x08;
    }
    return r;
}