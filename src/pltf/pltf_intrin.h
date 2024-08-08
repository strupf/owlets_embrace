// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_INTRIN_H
#define PLTF_INTRIN_H

#include "pltf_types.h"

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

#if defined(PLTF_PD_HW)
// clang-format off
#define ASM              __asm volatile
#define ASM1(I, R, A)    ASM(#I " %0, %1"     : "=r"(R) : "r"(A))
#define ASM2(I, R, A, B) ASM(#I " %0, %1, %2" : "=r"(R) : "r"(A), "r"(B))
// clang-format on

static inline u32 bswap32(u32 v)
{
    u32 r = 0;
    ASM1(rev, r, v);
    return r;
}

static inline i32 ssat16(i32 x)
{
    u32 r = 0;
    u32 i = (u32)x;
    ASM("ssat %0, %1, %2"
        : "=r"(r)
        : "I"(16), "r"(i));
    return (i32)r;
}

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

static inline i32 i16x2_muad(i16x2 x, i16x2 y)
{
    i32 r = 0;
    ASM2(smuad, r, x, y);
    return r;
}

static inline i32 i16x2_muadx(i16x2 x, i16x2 y)
{
    i32 r = 0;
    ASM2(smuadx, r, x, y);
    return r;
}

static inline i32 i16x2_musd(i16x2 x, i16x2 y)
{
    i32 r = 0;
    ASM2(smusd, r, x, y);
    return r;
}

static inline i32 i16x2_musdx(i16x2 x, i16x2 y)
{
    i32 r = 0;
    ASM2(smusdx, r, x, y);
    return r;
}

static inline u16x2 u16x2_add(u16x2 x, u16x2 y)
{
    u16x2 r = 0;
    ASM2(uadd16, r, x, y);
    return r;
}

static inline u16x2 u16x2_sadd(u16x2 x, u16x2 y)
{
    u16x2 r = 0;
    ASM2(uqadd16, r, x, y);
    return r;
}

static inline u16x2 u16x2_sub(u16x2 x, u16x2 y)
{
    u16x2 r = 0;
    ASM2(usub16, r, x, y);
    return r;
}

static inline u16x2 u16x2_ssub(u16x2 x, u16x2 y)
{
    u16x2 r = 0;
    ASM2(uqsub16, r, x, y);
    return r;
}

static inline i16x2 i16x2_add(i16x2 x, i16x2 y)
{
    i16x2 r = 0;
    ASM2(sadd16, r, x, y);
    return r;
}

static inline i16x2 i16x2_sadd(i16x2 x, i16x2 y)
{
    i16x2 r = 0;
    ASM2(qadd16, r, x, y);
    return r;
}

static inline i16x2 i16x2_sub(i16x2 x, i16x2 y)
{
    i16x2 r = 0;
    ASM2(ssub16, r, x, y);
    return r;
}

static inline i16x2 i16x2_ssub(i16x2 x, i16x2 y)
{
    i16x2 r = 0;
    ASM2(qsub16, r, x, y);
    return r;
}

static inline u8x4 u8x4_add(u8x4 x, u8x4 y)
{
    u8x4 r = 0;
    ASM2(uadd8, r, x, y);
    return r;
}

static inline u8x4 u8x4_sadd(u8x4 x, u8x4 y)
{
    u8x4 r = 0;
    ASM2(uqadd8, r, x, y);
    return r;
}

static inline u8x4 u8x4_sub(u8x4 x, u8x4 y)
{
    u8x4 r = 0;
    ASM2(usub8, r, x, y);
    return r;
}

static inline u8x4 u8x4_ssub(u8x4 x, u8x4 y)
{
    u8x4 r = 0;
    ASM2(uqsub8, r, x, y);
    return r;
}

static inline i8x4 i8x4_add(i8x4 x, i8x4 y)
{
    i8x4 r = 0;
    ASM2(sadd8, r, x, y);
    return r;
}

static inline i8x4 i8x4_sadd(i8x4 x, i8x4 y)
{
    i8x4 r = 0;
    ASM2(qadd8, r, x, y);
    return r;
}

static inline i8x4 i8x4_sub(i8x4 x, i8x4 y)
{
    i8x4 r = 0;
    ASM2(ssub8, r, x, y);
    return r;
}

static inline i8x4 i8x4_ssub(i8x4 x, i8x4 y)
{
    i8x4 r = 0;
    ASM2(qsub8, r, x, y);
    return r;
}

static inline u32 clz32(u32 v)
{
    u32 r = 0;
    ASM1(clz, r, v);
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

static inline i32 i16x2_muad(i16x2 x, i16x2 y)
{
    i16 a[2] = {0};
    i16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i32 r = (i32)a[0] * (i32)b[0] + (i32)a[1] * (i32)b[1];
    return r;
}

static inline i32 i16x2_muadx(i16x2 x, i16x2 y)
{
    i16 a[2] = {0};
    i16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i32 r = (i32)a[0] * (i32)b[1] + (i32)a[1] * (i32)b[0];
    return r;
}

static inline i32 i16x2_musd(i16x2 x, i16x2 y)
{
    i16 a[2] = {0};
    i16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i32 r = (i32)a[0] * (i32)b[0] - (i32)a[1] * (i32)b[1];
    return r;
}

static inline i32 i16x2_musdx(i16x2 x, i16x2 y)
{
    i16 a[2] = {0};
    i16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i32 r = (i32)a[0] * (i32)b[1] - (i32)a[1] * (i32)b[0];
    return r;
}

static inline u16x2 u16x2_add(u16x2 x, u16x2 y)
{
    u16x2 r = {0};
    u16 a[2] = {0};
    u16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    u16 z[2] = {a[0] + b[0],
                a[1] + b[1]};
    mcpy(&r, z, 4);
    return r;
}

static inline u16x2 u16x2_sadd(u16x2 x, u16x2 y)
{
    u16x2 r = {0};
    u16 a[2] = {0};
    u16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    u16 z[2] = {u16_sat((i32)a[0] + (i32)b[0]),
                u16_sat((i32)a[1] + (i32)b[1])};
    mcpy(&r, z, 4);
    return r;
}

static inline u16x2 u16x2_sub(u16x2 x, u16x2 y)
{
    u16x2 r = {0};
    u16 a[2] = {0};
    u16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    u16 z[2] = {a[0] - b[0],
                a[1] - b[1]};
    mcpy(&r, z, 4);
    return r;
}

static inline u16x2 u16x2_ssub(u16x2 x, u16x2 y)
{
    u16x2 r = {0};
    u16 a[2] = {0};
    u16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    u16 z[2] = {u16_sat((i32)a[0] - (i32)b[0]),
                u16_sat((i32)a[1] - (i32)b[1])};
    mcpy(&r, z, 4);
    return r;
}

static inline i16x2 i16x2_add(i16x2 x, i16x2 y)
{
    i16x2 r = {0};
    i16 a[2] = {0};
    i16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i16 z[2] = {a[0] + b[0],
                a[1] + b[1]};
    mcpy(&r, z, 4);
    return r;
}

static inline i16x2 i16x2_sadd(i16x2 x, i16x2 y)
{
    i16x2 r = {0};
    i16 a[2] = {0};
    i16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i16 z[2] = {i16_sat((i32)a[0] + (i32)b[0]),
                i16_sat((i32)a[1] + (i32)b[1])};
    mcpy(&r, z, 4);
    return r;
}

static inline i16x2 i16x2_sub(i16x2 x, i16x2 y)
{
    i16x2 r = {0};
    i16 a[2] = {0};
    i16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i16 z[2] = {a[0] - b[0],
                a[1] - b[1]};
    mcpy(&r, z, 4);
    return r;
}

static inline i16x2 i16x2_ssub(i16x2 x, i16x2 y)
{
    i16x2 r = {0};
    i16 a[2] = {0};
    i16 b[2] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i16 z[2] = {i16_sat((i32)a[0] - (i32)b[0]),
                i16_sat((i32)a[1] - (i32)b[1])};
    mcpy(&r, z, 4);
    return r;
}

static inline u8x4 u8x4_add(u8x4 x, u8x4 y)
{
    u8x4 r = {0};
    u8 a[4] = {0};
    u8 b[4] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    u8 z[4] = {a[0] + b[0],
               a[1] + b[1],
               a[2] + b[2],
               a[3] + b[3]};
    mcpy(&r, z, 4);
    return r;
}

static inline u8x4 u8x4_sadd(u8x4 x, u8x4 y)
{
    u8x4 r = {0};
    u8 a[4] = {0};
    u8 b[4] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    u8 z[4] = {u8_sat((i32)a[0] + (i32)b[0]),
               u8_sat((i32)a[1] + (i32)b[1]),
               u8_sat((i32)a[2] + (i32)b[2]),
               u8_sat((i32)a[3] + (i32)b[3])};
    mcpy(&r, z, 4);
    return r;
}

static inline u8x4 u8x4_sub(u8x4 x, u8x4 y)
{
    u8x4 r = {0};
    u8 a[4] = {0};
    u8 b[4] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    u8 z[4] = {a[0] - b[0],
               a[1] - b[1],
               a[2] - b[2],
               a[3] - b[3]};
    mcpy(&r, z, 4);
    return r;
}

static inline u8x4 u8x4_ssub(u8x4 x, u8x4 y)
{
    u8x4 r = {0};
    u8 a[4] = {0};
    u8 b[4] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    u8 z[4] = {u8_sat((i32)a[0] - (i32)b[0]),
               u8_sat((i32)a[1] - (i32)b[1]),
               u8_sat((i32)a[2] - (i32)b[2]),
               u8_sat((i32)a[3] - (i32)b[3])};
    mcpy(&r, z, 4);
    return r;
}

static inline i8x4 i8x4_add(i8x4 x, i8x4 y)
{
    i8x4 r = {0};
    i8 a[4] = {0};
    i8 b[4] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i8 z[4] = {a[0] + b[0],
               a[1] + b[1],
               a[2] + b[2],
               a[3] + b[3]};
    mcpy(&r, z, 4);
    return r;
}

static inline i8x4 i8x4_sadd(i8x4 x, i8x4 y)
{
    i8x4 r = {0};
    i8 a[4] = {0};
    i8 b[4] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i8 z[4] = {i8_sat((i32)a[0] + (i32)b[0]),
               i8_sat((i32)a[1] + (i32)b[1]),
               i8_sat((i32)a[2] + (i32)b[2]),
               i8_sat((i32)a[3] + (i32)b[3])};
    mcpy(&r, z, 4);
    return r;
}

static inline i8x4 i8x4_sub(i8x4 x, i8x4 y)
{
    i8x4 r = {0};
    i8 a[4] = {0};
    i8 b[4] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i8 z[4] = {a[0] - b[0],
               a[1] - b[1],
               a[2] - b[2],
               a[3] - b[3]};
    mcpy(&r, z, 4);
    return r;
}

static inline i8x4 i8x4_ssub(i8x4 x, i8x4 y)
{
    i8x4 r = {0};
    i8 a[4] = {0};
    i8 b[4] = {0};
    mcpy(a, &x, 4);
    mcpy(b, &y, 4);
    i8 z[4] = {i8_sat((i32)a[0] - (i32)b[0]),
               i8_sat((i32)a[1] - (i32)b[1]),
               i8_sat((i32)a[2] - (i32)b[2]),
               i8_sat((i32)a[3] - (i32)b[3])};
    mcpy(&r, z, 4);
    return r;
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