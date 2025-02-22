// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 p;
    v2_i32 v;
    i32    vmax;
} steerer_s;

v2_i32 steer_seek(v2_i32 p,
                  v2_i32 v,
                  v2_i32 p_trg,
                  i32    vmax)
{
    v2_i32 e = v2_i32_sub(p_trg, p);
    e        = v2_i32_setlen(e, vmax);
    v2_i32 r = v2_i32_sub(e, v);
    return r;
}

v2_i32 steer_flee(v2_i32 p,
                  v2_i32 v,
                  v2_i32 p_trg,
                  i32    vmax)
{
    v2_i32 r = steer_seek(p, v, p_trg, vmax);
    return v2_i32_inv(r);
}

v2_i32 steer_arrival(v2_i32 p,
                     v2_i32 v,
                     v2_i32 p_trg,
                     i32    vmax,
                     i32    radius)
{
    v2_i32 e    = v2_i32_sub(p_trg, p);
    i32    l    = v2_i32_len(e);
    i32    vset = vmax;
    if (l < radius) {
        vset = (vmax * l) / radius;
    }
    e        = v2_i32_setlenl(e, l, vset);
    v2_i32 r = v2_i32_sub(e, v);
    return r;
}