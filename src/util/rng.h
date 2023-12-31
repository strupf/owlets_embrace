// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef RNG_H
#define RNG_H

#include "sys/sys_types.h"

static u32 RNG_seed = 213;

// [0, 4294967296]
static u32 rngs_u32(u32 *seed)
{
    u32 x = *seed;
    x ^= x << 13, x ^= x >> 17, x ^= x << 5;
    *seed = x;
    return x;
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
    return rngs_f32(&RNG_seed);
}

// [0, 4294967296]
static u32 rng_u32()
{
    return rngs_u32(&RNG_seed);
}

// [0, 4294967296]
static i32 rng_i32()
{
    return (i32)rng_u32(&RNG_seed);
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

static f32 rngr_f32(f32 lo, f32 hi)
{
    return lo + rng_f32() * (hi - lo);
}

#endif