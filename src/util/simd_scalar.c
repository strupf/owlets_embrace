// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "simd.h"

static inline i32 i16x2_muad(vec32 a, vec32 b)
{
    SIMD_BEG_I16(a);
    SIMD_BEG_I16(b);
    i32 r = a.i_16[0] * b.i_16[0] + a.i_16[1] * b.i_16[1];
    return r;
}

static inline i32 i16x2_musd(vec32 a, vec32 b)
{
    SIMD_BEG_I16(a);
    SIMD_BEG_I16(b);
    i32 r = a.i_16[0] * b.i_16[0] - a.i_16[1] * b.i_16[1];
    return r;
}

static inline i32 i16x2_muadx(vec32 a, vec32 b)
{
    SIMD_BEG_I16(a);
    SIMD_BEG_I16(b);
    i32 r = a.i_16[0] * b.i_16[1] + a.i_16[1] * b.i_16[0];
    return r;
}

static inline i32 i16x2_musdx(vec32 a, vec32 b)
{
    SIMD_BEG_I16(a);
    SIMD_BEG_I16(b);
    i32 r = a.i_16[0] * b.i_16[1] - a.i_16[1] * b.i_16[0];
    return r;
}

static inline vec32 i16x2_add(vec32 a, vec32 b)
{
    vec32 r = {0};
    SIMD_BEG_I16(a);
    SIMD_BEG_I16(b);
    r.i_16[0] = a.i_16[0] + b.i_16[0];
    r.i_16[1] = a.i_16[1] + b.i_16[1];
    SIMD_END_I16(r);
    return r;
}

static inline vec32 i16x2_sub(vec32 a, vec32 b)
{
    vec32 r = {0};
    SIMD_BEG_I16(a);
    SIMD_BEG_I16(b);
    r.i_16[0] = a.i_16[0] - b.i_16[0];
    r.i_16[1] = a.i_16[1] - b.i_16[1];
    SIMD_END_I16(r);
    return r;
}

static inline vec32 i16x2_adds(vec32 a, vec32 b)
{
    vec32 r = {0};
    SIMD_BEG_I16(a);
    SIMD_BEG_I16(b);
    r.i_16[0] = i16_sat((i32)a.i_16[0] + (i32)b.i_16[0]);
    r.i_16[1] = i16_sat((i32)a.i_16[1] + (i32)b.i_16[1]);
    SIMD_END_I16(r);
    return r;
}

static inline vec32 i16x2_subs(vec32 a, vec32 b)
{
    vec32 r = {0};
    SIMD_BEG_I16(a);
    SIMD_BEG_I16(b);
    r.i_16[0] = i16_sat((i32)a.i_16[0] - (i32)b.i_16[0]);
    r.i_16[1] = i16_sat((i32)a.i_16[1] - (i32)b.i_16[1]);
    SIMD_END_I16(r);
    return r;
}

static inline vec32 u16x2_add(vec32 a, vec32 b)
{
    vec32 r = {0};
    SIMD_BEG_U16(a);
    SIMD_BEG_U16(b);
    r.u_16[0] = a.u_16[0] + b.u_16[0];
    r.u_16[1] = a.u_16[1] + b.u_16[1];
    SIMD_END_U16(r);
    return r;
}

static inline vec32 u16x2_sub(vec32 a, vec32 b)
{
    vec32 r = {0};
    SIMD_BEG_U16(a);
    SIMD_BEG_U16(b);
    r.u_16[0] = a.u_16[0] - b.u_16[0];
    r.u_16[1] = a.u_16[1] - b.u_16[1];
    SIMD_END_U16(r);
    return r;
}

static inline vec32 u16x2_adds(vec32 a, vec32 b)
{
    vec32 r = {0};
    SIMD_BEG_U16(a);
    SIMD_BEG_U16(b);
    r.u_16[0] = u16_sat((i32)a.u_16[0] + (i32)b.u_16[0]);
    r.u_16[1] = u16_sat((i32)a.u_16[1] + (i32)b.u_16[1]);
    SIMD_END_U16(r);
    return r;
}

static inline vec32 u16x2_subs(vec32 a, vec32 b)
{
    vec32 r = {0};
    SIMD_BEG_U16(a);
    SIMD_BEG_U16(b);
    r.u_16[0] = u16_sat((i32)a.u_16[0] - (i32)b.u_16[0]);
    r.u_16[1] = u16_sat((i32)a.u_16[1] - (i32)b.u_16[1]);
    SIMD_END_U16(r);
    return r;
}