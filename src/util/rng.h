// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef RNG_H
#define RNG_H

#include "pltf/pltf.h"

#define RNG_MAX 65535

i32 rngs_i32(u32 *s); // [0, 65535]
i32 rng_i32();        // [0, 65535]
i32 rngs_i32_bound(u32 *s, i32 hi);
i32 rngsr_i32(u32 *seed, i32 lo, i32 hi);
i32 rngr_i32(i32 lo, i32 hi);
i32 rngr_sym_i32(i32 hi);
i32 rngsr_sym_i32(u32 *seed, i32 hi);
f32 rngs_f32(u32 *seed); // [0, 1)
f32 rng_f32();           // [0, 1)
f32 rngr_f32(f32 lo, f32 hi);
#endif