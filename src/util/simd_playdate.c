// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// ARM Cortex M7 SIMD instructions

#include "simd.h"

static inline i32 i16x2_muad(vec32 a, vec32 b)
{
    i32 r = 0;
    ASM2(smuad, r, a.u_32, b.u_32);
    return r;
}

static inline i32 i16x2_musd(vec32 a, vec32 b)
{
    i32 r = 0;
    ASM2(smusd, r, a.u_32, b.u_32);
    return r;
}

static inline i32 i16x2_muadx(vec32 a, vec32 b)
{
    i32 r = 0;
    ASM2(smuadx, r, a.u_32, b.u_32);
    return r;
}

static inline i32 i16x2_musdx(vec32 a, vec32 b)
{
    i32 r = 0;
    ASM2(smusdx, r, a.u_32, b.u_32);
    return r;
}

static inline vec32 i16x2_add(vec32 a, vec32 b)
{
    vec32 r = {0};
    ASM2(sadd16, r.u_32, a.u_32, b.u_32);
    return r;
}

static inline vec32 i16x2_sub(vec32 a, vec32 b)
{
    vec32 r = {0};
    ASM2(ssub16, r.u_32, a.u_32, b.u_32);
    return r;
}

static inline vec32 i16x2_adds(vec32 a, vec32 b)
{
    vec32 r = {0};
    ASM2(qadd16, r.u_32, a.u_32, b.u_32);
    return r;
}

static inline vec32 i16x2_subs(vec32 a, vec32 b)
{
    vec32 r = {0};
    ASM2(qsub16, r.u_32, a.u_32, b.u_32);
    return r;
}

static inline vec32 u16x2_add(vec32 a, vec32 b)
{
    vec32 r = {0};
    ASM2(uadd16, r.u_32, a.u_32, b.u_32);
    return r;
}

static inline vec32 u16x2_sub(vec32 a, vec32 b)
{
    vec32 r = {0};
    ASM2(usub16, r.u_32, a.u_32, b.u_32);
    return r;
}

static inline vec32 u16x2_adds(vec32 a, vec32 b)
{
    vec32 r = {0};
    ASM2(uqadd16, r.u_32, a.u_32, b.u_32);
    return r;
}

static inline vec32 u16x2_subs(vec32 a, vec32 b)
{
    vec32 r = {0};
    ASM2(uqsub16, r.u_32, a.u_32, b.u_32);
    return r;
}