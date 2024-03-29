// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SPM_H
#define SPM_H

#include "sys/sys_types.h"
#include "util/mem.h"

typedef struct {
    void *stack[16];
    i32   n_stack;
#ifdef SYS_DEBUG
    usize lowestleft;
#endif

    marena_s             m;
    align_CL mkilobyte_s mem[1024];
} SPM_s;

extern SPM_s         SPM;
extern const alloc_s spm_allocator;

void  spm_init();
void  spm_push();
void  spm_pop();
void *spm_alloc(usize s);
void *spm_allocz(usize s);
void *spm_alloc_rem(usize *s);
void  spm_reset();

#endif