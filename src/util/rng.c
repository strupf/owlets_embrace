// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "util/rng.h"
#include "util/mathfunc.h"

static u32 g_rng_seed = 314159265;

// xorshift
i32 rngs_i32(u32 *s)
{
    u32 x = (*s ? *s : 314159265); // seed non-zero
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;

    // scramble and return high 16 bits only (more random)
    return (i32)((x * 2463534241U) >> 16);
}

i32 rng_i32()
{
    return rngs_i32(&g_rng_seed);
}

i32 rngs_i32_bound(u32 *s, i32 hi)
{
    return (rngs_i32(s) % (hi + 1));
}

i32 rngsr_i32(u32 *seed, i32 lo, i32 hi)
{
    return i32_range(rngs_i32(seed), lo, hi);
}

i32 rngr_i32(i32 lo, i32 hi)
{
    return rngsr_i32(&g_rng_seed, lo, hi);
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
    return ((f32)rngs_i32(seed) * 0.000015259f);
}

f32 rng_f32()
{
    return rngs_f32(&g_rng_seed);
}

f32 rngr_f32(f32 lo, f32 hi)
{
    return lo + rng_f32() * (hi - lo);
}