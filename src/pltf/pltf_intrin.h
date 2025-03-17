// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_INTRIN_H
#define PLTF_INTRIN_H

#include "pltf_types.h"

#define i16x2_dot i16x2_smuad
#define i16x2_crs i16x2_smusdx
#if PLTF_PD_HW
#include <arm_acle.h>
typedef int16x2_t  i16x2;
typedef uint16x2_t u16x2;
typedef int8x4_t   i8x4;
typedef uint8x4_t  u8x4;

#define mcpy_t(T, D, S)                           \
    mcpy(__builtin_assume_aligned(D, alignof(T)), \
         __builtin_assume_aligned(S, alignof(T)), \
         sizeof(T))

#define i16x2_smuad  __smuad
#define i16x2_smuadx __smuadx
#define i16x2_smusd  __smusd
#define i16x2_smusdx __smusdx
#define i16x2_smlad  __smlad
#define i16x2_adds   __qadd16
#define i16x2_add    __sadd16
#define i16x2_sub    __ssub16
#define i8x4_add     __sadd8
#define i8x4_sub     __ssub8
#define ssat         __ssat

static inline u32 ror_u32(u32 x, i32 v)
{
    u32 r = 0;
    __asm("ror %0, %1, %2" : "=r"(r) : "r"(x), "r"(v));
    return r;
}

static inline i32 i16_adds(i32 a, i32 b)
{
    return (i16x2_adds(a, b) & 0xFFFF);
}

static inline u32 brev32(u32 v)
{
    u32 r = 0;
    __asm("rbit %0, %1" : "=r"(r) : "r"(v));
    return r;
}

static inline i32 smulwb(i32 a, i16x2 b)
{
    i32 r = 0;
    __asm("smulwb %0, %1, %2" : "=r"(r) : "r"(a), "r"((i32)b));
    return r;
}

#define smulwbs smulwb

static inline i32 smlawb(i32 a, i16x2 b, i32 c)
{
    i32 r = 0;
    __asm("smlawb %0, %1, %2, %3" : "=r"(r) : "r"(a), "r"(b), "r"(c));
    return r;
}

#define smlawbs smlawb

static inline i32 mul_q16(i32 a, i16 b)
{
    return smulwb(a, (i16x2)b);
}

#else
typedef struct {
    ALIGNAS(4)
    i16 v[2];
} i16x2;

typedef struct {
    ALIGNAS(4)
    u16 v[2];
} u16x2;

typedef struct {
    ALIGNAS(4)
    i8 v[4];
} i8x4;

typedef struct {
    ALIGNAS(4)
    u8 v[4];
} u8x4;

#define mcpy_t(T, D, S) mcpy(D, S, sizeof(T))

static inline u32 ror_u32(u32 x, i32 v)
{
    u32 r = ((u32)((u64)x >> v)) | ((u32)((u64)x << (32 - v)));
    return r;
}

static inline i32 ssat(i32 x, i32 b)
{
    i32 lo = -(1 << (b - 1));
    i32 hi = +(1 << (b - 1)) - 1;
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline i32 i16x2_smuad(i16x2 a, i16x2 b)
{
    i32 r = (i32)a.v[0] * b.v[0] + (i32)a.v[1] * b.v[1];
    return r;
}

static inline i32 i16x2_smuadx(i16x2 a, i16x2 b)
{
    i32 r = (i32)a.v[0] * b.v[1] + (i32)a.v[1] * b.v[0];
    return r;
}

static inline i32 i16x2_smusd(i16x2 a, i16x2 b)
{
    i32 r = (i32)a.v[0] * b.v[0] - (i32)a.v[1] * b.v[1];
    return r;
}

static inline i32 i16x2_smusdx(i16x2 a, i16x2 b)
{
    i32 r = (i32)a.v[0] * b.v[1] - (i32)a.v[1] * b.v[0];
    return r;
}

static inline i32 i16x2_smlad(i16x2 a, i16x2 b, i32 c)
{
    i32 r = (i32)a.v[0] * b.v[0] + (i32)a.v[1] * b.v[1] + c;
    return r;
}

static inline i16x2 i16x2_adds(i16x2 a, i16x2 b)
{
    i16x2 r = {ssat((i32)a.v[0] + b.v[0], 16),
               ssat((i32)a.v[1] + b.v[1], 16)};
    return r;
}

static inline i16x2 i16x2_add(i16x2 a, i16x2 b)
{
    i16x2 r = {a.v[0] + b.v[0], a.v[1] + b.v[1]};
    return r;
}

static inline i16x2 i16x2_sub(i16x2 a, i16x2 b)
{
    i16x2 r = {a.v[0] - b.v[0], a.v[1] - b.v[1]};
    return r;
}

static inline i8x4 i8x4_add(i8x4 a, i8x4 b)
{
    i8x4 r = {a.v[0] + b.v[0], a.v[1] + b.v[1],
              a.v[2] + b.v[2], a.v[3] + b.v[3]};
    return r;
}

static inline i8x4 i8x4_sub(i8x4 a, i8x4 b)
{
    i8x4 r = {a.v[0] - b.v[0], a.v[1] - b.v[1],
              a.v[2] - b.v[2], a.v[3] - b.v[3]};
    return r;
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

static inline i32 i16_adds(i32 a, i32 b)
{
    i32 r = ssat(a + b, 16);
    return r;
}

static inline i32 mul_q16(i32 a, i16 b)
{
    i32 r = (i32)(((i64)a * b) >> 16);
    return r;
}

static inline i32 smulwb(i32 a, i16x2 b)
{
    i32 r = (i32)(((i64)a * b.v[0]) >> 16);
    return r;
}

static inline i32 smulwbs(i32 a, i16 b)
{
    i32 r = (i32)(((i64)a * b) >> 16);
    return r;
}

static inline i32 smlawb(i32 a, i16x2 b, i32 c)
{
    i32 r = (i32)(((i64)a * b.v[0]) >> 16) + c;
    return r;
}

static inline i32 smlawbs(i32 a, i16 b, i32 c)
{
    i32 r = (i32)(((i64)a * b) >> 16) + c;
    return r;
}
#endif

#define SIMD_LD_IMPL(T)                     \
    static inline T T##_ld(const void *p)   \
    {                                       \
        T r = {0};                          \
        mcpy_t(T, &r, p);                   \
        return r;                           \
    }                                       \
                                            \
    static inline void T##_st(T v, void *p) \
    {                                       \
        mcpy_t(T, p, &v);                   \
    }

SIMD_LD_IMPL(i16x2)
SIMD_LD_IMPL(u16x2)
SIMD_LD_IMPL(i8x4)
SIMD_LD_IMPL(u8x4)

// BSWAP
#if defined(__GNUC__)
#define bswap32 __builtin_bswap32
#elif defined(_MSC_VER)
#define bswap32 _byteswap_ulong
#else
static u32 bswap32(u32 i)
{
    return (i >> 24) | ((i << 8) & 0xFF0000U) |
           (i << 24) | ((i >> 8) & 0x00FF00U);
}
#endif

// CLZ
#if defined(__GNUC__)
#define clz32 __builtin_clz // undefined for 0
#elif defined(_MSC_VER)
static i32 clz32(u32 v) // undefined for 0
{
    ulong m = (ulong)v;
    ulong r = 0;
    _BitScanReverse(&r, m);
    return (31 - (i32)r);
}
#else
static i32 clz32(u32 v)
{
    for (i32 i = 0; i < 32; i++) {
        if (v & (0x80000000U >> i)) return i;
    }
    return 32;
}
#endif

static inline i16x2 i16x2_shl(i16x2 v, i32 s)
{
    i16x2 r = {0};
    u32   u = 0;
    mcpy_t(u32, &u, &v);
    u = (u << s) & (0x0000FFFFU | (0xFFFF0000U << s)); // same as u16
    mcpy_t(u32, &r, &u);
    return r;
}

static inline i16x2 i16x2_shr(i16x2 v, i32 s)
{
    i16x2          r    = {0};
    ALIGNAS(4) i16 a[2] = {0};
    mcpy_t(u32, a, &v);
    a[0] >>= s, a[1] >>= s;
    mcpy_t(u32, &r, a);
    return r;
}

static inline u16x2 u16x2_shl(u16x2 v, i32 s)
{
    u16x2 r = {0};
    u32   u = 0;
    mcpy_t(u32, &u, &v);
    u = (u << s) & (0x0000FFFF | (0xFFFF0000 << s)); // same as u16
    mcpy_t(u32, &r, &u);
    return r;
}

static inline u16x2 u16x2_shr(u16x2 v, i32 s)
{
    u16x2 r = {0};
    u32   u = 0;
    mcpy_t(u32, &u, &v);
    u = (u >> s) & (0xFFFF0000 | (0x0000FFFF >> s));
    mcpy_t(u32, &r, &u);
    return r;
}

#endif