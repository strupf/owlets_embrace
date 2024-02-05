// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SYS_INTRIN_H
#define SYS_INTRIN_H

#include "sys_types.h"

#ifdef SYS_PD_HW
#define bswap32 __builtin_bswap32

static inline u32 clz32(u32 v)
{
    i32 r;
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