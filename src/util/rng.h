// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef RNG_H
#define RNG_H

#include "sys/sys_types.h"

// [0, 4294967295]
static inline u32 rngn_u32(u32 u)
{
    u32 x = u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

// [0, 4294967295]
static u32 rngs_u32(u32 *seed)
{
    u32 x = rngn_u32(*seed);
    *seed = x;
    return x;
}

// [0, 4294967295]
static u32 rng_u32()
{
    static u32 RNG_seed = 213;
    return rngs_u32(&RNG_seed);
}

static inline i32 rngs_i32(u32 *seed)
{
    return (i32)rngs_u32(seed);
}

// [0, 1]
static f32 rngs_f32(u32 *seed)
{
    return (rngs_u32(seed) / (f32)0xFFFFFFFFU);
}

// [0, 1]
static f32 rng_f32()
{
    return (rng_u32() / (f32)0xFFFFFFFFU);
}

// [0, 4294967295]
static i32 rng_i32()
{
    return (i32)rng_u32();
}

// [lo, hi]
static u32 rngr_u32(u32 lo, u32 hi)
{
    return lo + (rng_u32() % (hi - lo + 1));
}

// [lo, hi]
static u32 rngsr_u32(u32 *seed, u32 lo, u32 hi)
{
    return lo + (rngs_u32(seed) % (hi - lo + 1));
}

// [lo, hi]
static i32 rngr_i32(i32 lo, i32 hi)
{
    return lo + (rng_u32() % (hi - lo + 1));
}

// [lo, hi]
static i32 rngr_sym_i32(i32 hi)
{
    return rngr_i32(-hi, +hi);
}

static f32 rngr_f32(f32 lo, f32 hi)
{
    return lo + rng_f32() * (hi - lo);
}

#endif