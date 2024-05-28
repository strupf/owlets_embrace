// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_INTRIN_H
#define PLTF_INTRIN_H

#include "pltf_types.h"

#ifdef PLTF_PD_HW
#define bswap32 __builtin_bswap32

static inline i32 i32_sadd(i32 x, i32 y)
{
    i32 r;
    __asm("qadd %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline i32 i32_ssub(i32 x, i32 y)
{
    i32 r;
    __asm("qsub %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline u16x2 u16x2_add(u16x2 x, u16x2 y)
{
    u16x2 r;
    __asm("uadd16 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline u16x2 u16x2_sadd(u16x2 x, u16x2 y)
{
    u16x2 r;
    __asm("uqadd16 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline u16x2 u16x2_sub(u16x2 x, u16x2 y)
{
    u16x2 r;
    __asm("usub16 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline u16x2 u16x2_ssub(u16x2 x, u16x2 y)
{
    u16x2 r;
    __asm("uqsub16 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline i16x2 i16x2_add(i16x2 x, i16x2 y)
{
    i16x2 r;
    __asm("sadd16 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline i16x2 i16x2_sadd(i16x2 x, i16x2 y)
{
    i16x2 r;
    __asm("qadd16 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline i16x2 i16x2_sub(i16x2 x, i16x2 y)
{
    i16x2 r;
    __asm("ssub16 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline i16x2 i16x2_ssub(i16x2 x, i16x2 y)
{
    i16x2 r;
    __asm("qsub16 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline u8x4 u8x4_add(u8x4 x, u8x4 y)
{
    u8x4 r;
    __asm("uadd8 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline u8x4 u8x4_sadd(u8x4 x, u8x4 y)
{
    u8x4 r;
    __asm("uqadd8 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline u8x4 u8x4_sub(u8x4 x, u8x4 y)
{
    u8x4 r;
    __asm("usub8 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline u8x4 u8x4_ssub(u8x4 x, u8x4 y)
{
    u8x4 r;
    __asm("uqsub8 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline i8x4 i8x4_add(i8x4 x, i8x4 y)
{
    i8x4 r;
    __asm("sadd8 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline i8x4 i8x4_sadd(i8x4 x, i8x4 y)
{
    i8x4 r;
    __asm("qadd8 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline i8x4 i8x4_sub(i8x4 x, i8x4 y)
{
    i8x4 r;
    __asm("ssub8 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline i8x4 i8x4_ssub(i8x4 x, i8x4 y)
{
    i8x4 r;
    __asm("qsub8 %0, %1, %2"
          : "=r"(r)
          : "r"(x), "r"(y));
    return r;
}

static inline u32 clz32(u32 v)
{
    u32 r;
    __asm("clz %0, %1"
          : "=r"(r)
          : "r"(v));
    return r;
}

static inline u32 brev32(u32 v)
{
    u32 r;
    __asm("rbit %0, %1"
          : "=r"(r)
          : "r"(v));
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

static inline u16x2 u16x2_add(u16x2 x, u16x2 y)
{
    u16x2 r;
    u16  *a        = (u16 *)&x;
    u16  *b        = (u16 *)&y;
    ((u16 *)&r)[0] = a[0] + b[0];
    ((u16 *)&r)[1] = a[1] + b[1];
    return r;
}

static inline u16x2 u16x2_sadd(u16x2 x, u16x2 y)
{
    u16x2 r;
    u16  *a        = (u16 *)&x;
    u16  *b        = (u16 *)&y;
    ((u16 *)&r)[0] = u16_sat((i32)a[0] + (i32)b[0]);
    ((u16 *)&r)[1] = u16_sat((i32)a[1] + (i32)b[1]);
    return r;
}

static inline u16x2 u16x2_sub(u16x2 x, u16x2 y)
{
    u16x2 r;
    u16  *a        = (u16 *)&x;
    u16  *b        = (u16 *)&y;
    ((u16 *)&r)[0] = a[0] - b[0];
    ((u16 *)&r)[1] = a[1] - b[1];
    return r;
}

static inline u16x2 u16x2_ssub(u16x2 x, u16x2 y)
{
    u16x2 r;
    u16  *a        = (u16 *)&x;
    u16  *b        = (u16 *)&y;
    ((u16 *)&r)[0] = u16_sat((i32)a[0] - (i32)b[0]);
    ((u16 *)&r)[1] = u16_sat((i32)a[1] - (i32)b[1]);
    return r;
}

static inline i16x2 i16x2_add(i16x2 x, i16x2 y)
{
    i16x2 r;
    i16  *a        = (i16 *)&x;
    i16  *b        = (i16 *)&y;
    ((i16 *)&r)[0] = a[0] + b[0];
    ((i16 *)&r)[1] = a[1] + b[1];
    return r;
}

static inline i16x2 i16x2_sadd(i16x2 x, i16x2 y)
{
    i16x2 r;
    i16  *a        = (i16 *)&x;
    i16  *b        = (i16 *)&y;
    ((i16 *)&r)[0] = i16_sat((i32)a[0] + (i32)b[0]);
    ((i16 *)&r)[1] = i16_sat((i32)a[1] + (i32)b[1]);
    return r;
}

static inline i16x2 i16x2_sub(i16x2 x, i16x2 y)
{
    i16x2 r;
    i16  *a        = (i16 *)&x;
    i16  *b        = (i16 *)&y;
    ((i16 *)&r)[0] = a[0] - b[0];
    ((i16 *)&r)[1] = a[1] - b[1];
    return r;
}

static inline i16x2 i16x2_ssub(i16x2 x, i16x2 y)
{
    i16x2 r;
    i16  *a        = (i16 *)&x;
    i16  *b        = (i16 *)&y;
    ((i16 *)&r)[0] = i16_sat((i32)a[0] - (i32)b[0]);
    ((i16 *)&r)[1] = i16_sat((i32)a[1] - (i32)b[1]);
    return r;
}

static inline u8x4 u8x4_add(u8x4 x, u8x4 y)
{
    u8x4 r;
    u8  *a        = (u8 *)&x;
    u8  *b        = (u8 *)&y;
    ((u8 *)&r)[0] = a[0] + b[0];
    ((u8 *)&r)[1] = a[1] + b[1];
    ((u8 *)&r)[2] = a[2] + b[2];
    ((u8 *)&r)[3] = a[3] + b[3];
    return r;
}

static inline u8x4 u8x4_sadd(u8x4 x, u8x4 y)
{
    u8x4 r;
    u8  *a        = (u8 *)&x;
    u8  *b        = (u8 *)&y;
    ((u8 *)&r)[0] = u8_sat((i32)a[0] + (i32)b[0]);
    ((u8 *)&r)[1] = u8_sat((i32)a[1] + (i32)b[1]);
    ((u8 *)&r)[2] = u8_sat((i32)a[2] + (i32)b[2]);
    ((u8 *)&r)[3] = u8_sat((i32)a[3] + (i32)b[3]);
    return r;
}

static inline u8x4 u8x4_sub(u8x4 x, u8x4 y)
{
    u8x4 r;
    u8  *a        = (u8 *)&x;
    u8  *b        = (u8 *)&y;
    ((u8 *)&r)[0] = a[0] - b[0];
    ((u8 *)&r)[1] = a[1] - b[1];
    ((u8 *)&r)[2] = a[2] - b[2];
    ((u8 *)&r)[3] = a[3] - b[3];
    return r;
}

static inline u8x4 u8x4_ssub(u8x4 x, u8x4 y)
{
    u8x4 r;
    u8  *a        = (u8 *)&x;
    u8  *b        = (u8 *)&y;
    ((u8 *)&r)[0] = u8_sat((i32)a[0] - (i32)b[0]);
    ((u8 *)&r)[1] = u8_sat((i32)a[1] - (i32)b[1]);
    ((u8 *)&r)[2] = u8_sat((i32)a[2] - (i32)b[2]);
    ((u8 *)&r)[3] = u8_sat((i32)a[3] - (i32)b[3]);
    return r;
}

static inline i8x4 i8x4_add(i8x4 x, i8x4 y)
{
    i8x4 r;
    i8  *a        = (i8 *)&x;
    i8  *b        = (i8 *)&y;
    ((i8 *)&r)[0] = a[0] + b[0];
    ((i8 *)&r)[1] = a[1] + b[1];
    ((i8 *)&r)[2] = a[2] + b[2];
    ((i8 *)&r)[3] = a[3] + b[3];
    return r;
}

static inline i8x4 i8x4_sadd(i8x4 x, i8x4 y)
{
    i8x4 r;
    i8  *v        = (i8 *)&r;
    i8  *a        = (i8 *)&x;
    i8  *b        = (i8 *)&y;
    ((i8 *)&r)[0] = i8_sat((i32)a[0] + (i32)b[0]);
    ((i8 *)&r)[1] = i8_sat((i32)a[1] + (i32)b[1]);
    ((i8 *)&r)[2] = i8_sat((i32)a[2] + (i32)b[2]);
    ((i8 *)&r)[3] = i8_sat((i32)a[3] + (i32)b[3]);
    return r;
}

static inline i8x4 i8x4_sub(i8x4 x, i8x4 y)
{
    i8x4 r;
    i8  *a        = (i8 *)&x;
    i8  *b        = (i8 *)&y;
    ((i8 *)&r)[0] = a[0] - b[0];
    ((i8 *)&r)[1] = a[1] - b[1];
    ((i8 *)&r)[2] = a[2] - b[2];
    ((i8 *)&r)[3] = a[3] - b[3];
    return r;
}

static inline i8x4 i8x4_ssub(i8x4 x, i8x4 y)
{
    i8x4 r;
    i8  *a        = (i8 *)&x;
    i8  *b        = (i8 *)&y;
    ((i8 *)&r)[0] = i8_sat((i32)a[0] - (i32)b[0]);
    ((i8 *)&r)[1] = i8_sat((i32)a[1] - (i32)b[1]);
    ((i8 *)&r)[2] = i8_sat((i32)a[2] - (i32)b[2]);
    ((i8 *)&r)[3] = i8_sat((i32)a[3] - (i32)b[3]);
    return r;
}

static u32 bswap32(u32 i)
{
    return (i >> 24) | ((i << 8) & 0xFF0000U) |
           (i << 24) | ((i >> 8) & 0x00FF00U);
}

static i32 clz32(u32 v)
{
    for (int i = 0; i < 32; i++) {
        if (v & (0x80000000U >> i)) return i;
    }
    return 31;
}

static u32 brev32(u32 x)
{
    u32 r = x;
    int s = 31;
    for (u32 v = x >> 1; v; v >>= 1) {
        r <<= 1;
        r |= v & 1;
        s--;
    }
    return (r << s);
}
#endif

#endif