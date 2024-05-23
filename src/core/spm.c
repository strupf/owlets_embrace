// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "spm.h"

SPM_s SPM;

static void  *spm_alloc_ctx(void *ctx, usize s);
const alloc_s spm_allocator = {spm_alloc_ctx, NULL};

static void *spm_alloc_ctx(void *ctx, usize s)
{
    return spm_alloc(s);
}

void spm_init()
{
    marena_init(&SPM.m, SPM.mem, sizeof(SPM.mem));
#ifdef SYS_DEBUG
    SPM.lowestleft = sizeof(SPM.mem);
#endif
}

void spm_push()
{

    SPM.stack[SPM.n_stack++] = marena_state(&SPM.m);
}

void spm_pop()
{
    assert(0 < SPM.n_stack);
    void *p = SPM.stack[--SPM.n_stack];
    marena_reset_to(&SPM.m, p);
}

void *spm_alloc(usize s)
{
    void *mem = marena_alloc(&SPM.m, s);
    assert(mem);
#ifdef SYS_DEBUG
    usize rem = marena_size_rem(&SPM.m);
    if (rem < SPM.lowestleft) {
        SPM.lowestleft = rem;
        sys_printf("+++ lowest SPM left: %u kb\n", (u32)(rem / 1024));
    }
#endif
    return mem;
}

void *spm_allocz(usize s)
{
    void *mem = spm_alloc(s);
    if (!mem) return NULL;
    memset(mem, 0, s);
    return mem;
}

void *spm_alloc_rem(usize *s)
{
    return marena_alloc_rem(&SPM.m, s);
}

void spm_reset()
{
    marena_reset(&SPM.m);
    SPM.n_stack = 0;
}