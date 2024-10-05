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
    memarena_init(&SPM.m, SPM.mem, sizeof(SPM.mem));
    SPM.lowestleft = sizeof(SPM.mem);
}

void spm_push()
{

    SPM.stack[SPM.n_stack++] = memarena_state(&SPM.m);
}

void spm_pop()
{
    assert(0 < SPM.n_stack);
    void *p = SPM.stack[--SPM.n_stack];
    memarena_reset_to(&SPM.m, p);
}

void spm_align(usize alignment)
{
    memarena_align(&SPM.m, alignment);
}

void *spm_alloc(usize s)
{
    void *mem = memarena_alloc(&SPM.m, s);
    assert(mem);
#ifdef PLTF_DEBUG
    usize rem = memarena_size_rem(&SPM.m);
    if (!SPM.lowestleft_disabled && rem < SPM.lowestleft) {
        SPM.lowestleft = rem;
        pltf_log("+++ lowest SPM left: %u kb\n", (u32)(rem / 1024));
    }
#endif
    return mem;
}

void *spm_allocz(usize s)
{
    void *mem = spm_alloc(s);
    if (mem) {
        mclr(mem, s);
    } else {
        BAD_PATH
    }
    return mem;
}

void *spm_alloc_aligned(usize s, usize alignment)
{
    spm_align(alignment);
    return spm_alloc(s);
}

void *spm_allocz_aligned(usize s, usize alignment)
{
    spm_align(alignment);
    return spm_allocz(s);
}

void *spm_alloc_rem(usize *s)
{
    return memarena_alloc_rem(&SPM.m, s);
}

void spm_reset()
{
    memarena_reset(&SPM.m);
    SPM.n_stack = 0;
}