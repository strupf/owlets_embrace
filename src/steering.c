// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

v2_i32 steer_seek(v2_i32 p, v2_i32 v, v2_i32 p_trg, i32 vmax)
{
    v2_i32 e = v2_i32_sub(p_trg, p);
    e        = v2_i32_setlen_fast(e, vmax);
    v2_i32 r = v2_i32_sub(e, v);
    return r;
}

v2_i32 steer_flee(v2_i32 p, v2_i32 v, v2_i32 p_trg, i32 vmax)
{
    v2_i32 r = steer_seek(p, v, p_trg, vmax);
    return v2_i32_inv(r);
}

v2_i32 steer_arrival(v2_i32 p, v2_i32 v, v2_i32 p_trg, i32 vmax, i32 radius)
{
    v2_i32 e    = v2_i32_sub(p_trg, p);
    i32    l    = v2_i32_len_appr(e);
    i32    vset = vmax;
    if (l < radius) {
        l    = min_i32((l * 259) >> 8, radius);
        vset = (vmax * l) / radius;
    }
    e        = v2_i32_setlenl_small(e, l, vset);
    v2_i32 r = v2_i32_sub(e, v);
    return r;
}

v2_f32 steerf_seek(v2_f32 p, v2_f32 v, v2_f32 p_trg, f32 vmax)
{
    v2_f32 e = v2f_sub(p_trg, p);
    e        = v2f_setlen(e, vmax);
    v2_f32 r = v2f_sub(e, v);
    return r;
}

v2_f32 steerf_flee(v2_f32 p, v2_f32 v, v2_f32 p_trg, f32 vmax)
{
    v2_f32 r = steerf_seek(p, v, p_trg, vmax);
    return v2f_inv(r);
}

v2_f32 steerf_arrival(v2_f32 p, v2_f32 v, v2_f32 p_trg, f32 vmax, f32 radius)
{
    v2_f32 e    = v2f_sub(p_trg, p);
    f32    l    = v2f_len(e);
    f32    vset = vmax;
    if (l < radius) {
        vset = (vmax * l) / radius;
    }
    vset     = max_f32(vset, 0.01f);
    e        = v2f_setlenl(e, l, vset);
    v2_f32 r = v2f_sub(e, v);
    return r;
}