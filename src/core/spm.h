// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SPM_H
#define SPM_H

#include "pltf/pltf.h"
#include "util/marena.h"

typedef struct {
#if PLTF_DEV_ENV
    bool32 lowestleft_disabled;
    usize  lowestleft;
#endif
    void    *stack[8];
    u32      n_stack;
    marena_s m;
    ALIGNAS(4)
    byte mem[MKILOBYTE(1024)];
} spm_s;

extern const alloc_s spm_allocator;

#define spm_alloct(T, N)  (T *)spm_alloc_aligned(sizeof(T) * (N), ALIGNOF(T))
#define spm_alloctz(T, N) (T *)spm_allocz_aligned(sizeof(T) * (N), ALIGNOF(T))

void  spm_init();
void  spm_push();
void  spm_pop();
void  spm_align(usize alignment);
void *spm_alloc(usize s);
void *spm_allocz(usize s);
void *spm_alloc_aligned(usize s, usize alignment);
void *spm_allocz_aligned(usize s, usize alignment);
void *spm_alloc_rem(usize *s);
void  spm_reset(void *p);

#endif