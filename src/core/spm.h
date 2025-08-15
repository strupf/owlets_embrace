// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// (s)cratch(p)ad (m)emory
// memory arena for temporary memory allocations

#ifndef SPM_H
#define SPM_H

#include "pltf/pltf.h"
#include "util/marena.h"

typedef struct spm_s {
    ALIGNAS(32)
    void    *stack[6];
    u32      n_stack;
    marena_s m;
#if PLTF_DEV_ENV
    bool32 lowestleft_disabled;
    usize  lowestleft;
#endif
} spm_s;

#define spm_alloct(T)     (T *)spm_alloc_aligned(sizeof(T), ALIGNOF(T))
#define spm_alloctn(T, N) (T *)spm_alloc_aligned(sizeof(T) * (N), ALIGNOF(T))
#define spm_alloctz(T, N) (T *)spm_allocz_aligned(sizeof(T) * (N), ALIGNOF(T))

void        spm_init(void *buf, usize bsize);
void        spm_push();
void        spm_pop();
void        spm_align(usize alignment);
void       *spm_alloc(usize s);
void       *spm_allocz(usize s);
void       *spm_alloc_aligned(usize s, usize alignment);
void       *spm_allocz_aligned(usize s, usize alignment);
void       *spm_alloc_rem(usize *s);
void        spm_reset(void *p);
allocator_s spm_allocator();

#endif