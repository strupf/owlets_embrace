// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef RNG_H
#define RNG_H

#include "pltf/pltf.h"

// [0, 4294967295]
u32 rngs_u32(u32 *s);
u32 rng_u32();
i32 rngs_i32(u32 *seed);
i32 rng_i32();
u32 rngs_u32_bound(u32 *s, u32 hi);
u32 rng_u32_bound(u32 hi);
i32 rngsr_i32(u32 *seed, i32 lo, i32 hi);
i32 rngr_i32(i32 lo, i32 hi);
u32 rngsr_u32(u32 *seed, u32 lo, u32 hi);
u32 rngr_u32(u32 lo, u32 hi);
i32 rngr_sym_i32(i32 hi);
i32 rngsr_sym_i32(u32 *seed, i32 hi);
f32 rngs_f32(u32 *seed);
f32 rng_f32();
f32 rngr_f32(f32 lo, f32 hi);
#endif