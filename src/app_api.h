// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef APP_API_H
#define APP_API_H

#include "pltf/pltf.h"

// allocate persistent memory
void       *app_alloc(usize s);
void       *app_alloc_aligned(usize s, usize alignment);
void       *app_alloc_aligned_ctx(void *ctx, usize s, usize alignment);
void       *app_alloc_aligned_rem(usize alignment, usize *rem);
allocator_s app_allocator();
#define app_alloct(T)     app_alloc_aligned(sizeof(T), ALIGNOF(T))
#define app_alloctn(T, N) app_alloc_aligned((N) * sizeof(T), ALIGNOF(T))

void app_crank_requested(b32 enable);

#endif
