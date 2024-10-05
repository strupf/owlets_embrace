// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "rng.h"

static u32 g_rng_seed = 213;

// Originally based on good ol' xorshift. However, when placing tiles using
// rng() with x and y as an input sometimes there were long strides of the
// same tiles -> repeating. Thus I needed a better PRNG.
//
// www.pcg-random.org
// PCG, rxs_m_xs_32
// github.com/imneme/pcg-cpp/blob/master/include/pcg_random.hpp#L947
u32 rngs_u32(u32 *s)
{
    u32 x = *s;
    x ^= x >> ((x >> 28) + 4);
    x *= 277803737U;
    *s = x;
    return ((x >> 22) ^ x);
}

u32 rng_u32()
{
    return rngs_u32(&g_rng_seed);
}

i32 rngs_i32(u32 *seed)
{
    return (i32)rngs_u32(seed);
}

i32 rng_i32()
{
    return (i32)rng_u32();
}

u32 rngs_u32_bound(u32 *s, u32 hi)
{
    if (hi == U32_MAX) return rngs_u32(s);
    return (rngs_u32(s) % (hi + 1));
}

u32 rng_u32_bound(u32 hi)
{
    return rngs_u32_bound(&g_rng_seed, hi);
}

i32 rngsr_i32(u32 *seed, i32 lo, i32 hi)
{
    return lo + (i32)rngs_u32_bound(seed, (u32)(hi - lo));
}

i32 rngr_i32(i32 lo, i32 hi)
{
    return rngsr_i32(&g_rng_seed, lo, hi);
}

u32 rngsr_u32(u32 *seed, u32 lo, u32 hi)
{
    return lo + rngs_u32_bound(seed, hi - lo);
}

u32 rngr_u32(u32 lo, u32 hi)
{

    return rngsr_u32(&g_rng_seed, lo, hi);
}

i32 rngr_sym_i32(i32 hi)
{
    return rngr_i32(-hi, +hi);
}

i32 rngsr_sym_i32(u32 *seed, i32 hi)
{
    return rngsr_i32(seed, -hi, +hi);
}

f32 rngs_f32(u32 *seed)
{
    return (rngs_u32(seed) / (f32)0xFFFFFFFFU);
}

f32 rng_f32()
{
    return rngs_f32(&g_rng_seed);
}

f32 rngr_f32(f32 lo, f32 hi)
{
    return lo + rng_f32() * (hi - lo);
}