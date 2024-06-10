// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SPM_H
#define SPM_H

#include "pltf/pltf.h"
#include "util/mem.h"

typedef struct {
    void *stack[16];
    i32   n_stack;
    usize lowestleft;

    marena_s    m;
    mkilobyte_s mem[1024];
} SPM_s;

extern SPM_s         SPM;
extern const alloc_s spm_allocator;

#define spm_alloct(TYPE, N)  (TYPE *)spm_alloc(sizeof(TYPE) * (N))
#define spm_alloctz(TYPE, N) (TYPE *)spm_allocz(sizeof(TYPE) * (N))

void  spm_init();
void  spm_push();
void  spm_pop();
void *spm_alloc(usize s);
void *spm_allocz(usize s);
void *spm_alloc_rem(usize *s);
void  spm_reset();

#endif