// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_INTRIN_H
#define PLTF_INTRIN_H

#include "pltf_types.h"
#include "util/simd.h"

#if 0
static inline i16x2 i16x2_shl(i16x2 v, i16x2 s)
{
#if 0
    i16x2 r    = {0};
    i16 a[2] = {0};
    mcpy(a, &v, 4);
    i16 z[2] = {a[0] << s,
                a[1] << s};
    mcpy(&r, z, 4);
#else
    u32 r = ((u32)v << s) & (0x0000FFFFU | (0xFFFF0000U << s)); // same as u16
#endif
    return (i16x2)r;
}

static inline i16x2 i16x2_shr(i16x2 v, i16x2 s)
{
    i16x2 r    = {0};
    i16   a[2] = {0};
    mcpy(a, &v, 4);
    i16 z[2] = {a[0] >> s,
                a[1] >> s};
    mcpy(&r, z, 4);
    return r;
}

static inline u16x2 u16x2_shl(u16x2 v, i32 s)
{
    u32 r = ((u32)v << s) & (0x0000FFFFU | (0xFFFF0000U << s));
    return (u16x2)r;
}

static inline u16x2 u16x2_shr(u16x2 v, i32 s)
{
    u32 r = ((u32)v >> s) & (0xFFFF0000U | (0x0000FFFFU >> s));
    return (u16x2)r;
}
#endif

#if defined(PLTF_PD_HW)
#define bswap32   __builtin_bswap32
#define clz32     __builtin_clz
#define ssat16(X) __ssat(X, 16)

static inline u32 rotr(u32 v, u32 rot)
{
    u32 r = 0;
    ASM2(ror, r, v, rot);
    return r;
}

static inline i32 i32_sadd(i32 x, i32 y)
{
    i32 r = 0;
    ASM2(qadd, r, x, y);
    return r;
}

static inline i32 i32_ssub(i32 x, i32 y)
{
    i32 r = 0;
    ASM2(qsub, r, x, y);
    return r;
}

static inline u32 brev32(u32 v)
{
    u32 r = 0;
    ASM1(rbit, r, v);
    return r;
}

static u32 rotr32(u32 x, u32 rot)
{
    u32 r = 0;
    ASM2(ror, r, x, rot);
    return r;
}

static u32 rotr16(u16 x, u32 rot)
{
    u16 r = 0;
    ASM2(ror, r, x, rot);
    return r;
}

#else

static inline i32 i32_sadd(i32 x, i32 y)
{
    return i32_sat((i64)x + (i64)y);
}

static inline i32 i32_ssub(i32 x, i32 y)
{
    return i32_sat((i64)x - (i64)y);
}

static inline i32 ssat16(i32 x)
{
    if (x < I16_MIN) return I16_MIN;
    if (x > I16_MAX) return I16_MAX;
    return x;
}

static u32 rotr16(u32 x, u32 r)
{
    return (x >> r) | (x << ((16 - r) & 15));
}

static u32 rotr32(u32 x, u32 r)
{
    return (x >> r) | (x << ((32 - r) & 31));
}

static u32 bswap32(u32 i)
{
    return (i >> 24) | ((i << 8) & 0xFF0000U) |
           (i << 24) | ((i >> 8) & 0x00FF00U);
}

static i32 clz32(u32 v)
{
    for (i32 i = 0; i < 32; i++) {
        if (v & (0x80000000U >> i)) return i;
    }
    return 31;
}

static u32 brev32(u32 x)
{
    u32 r = x;
    i32 s = 31;
    for (u32 v = x >> 1; v; v >>= 1) {
        r <<= 1;
        r |= v & 1;
        s--;
    }
    return (r << s);
}
#endif

#endif