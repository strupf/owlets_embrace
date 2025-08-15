// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/spm.h"
#include "app.h"

void spm_init(void *buf, usize bsize)
{
    marena_init(&APP.spm.m, buf, bsize);
    pltf_log("SPM init with: %u kb\n", (u32)bsize / 1024);
#if PLTF_DEV_ENV
    APP.spm.lowestleft = bsize;
#endif
}

void spm_push()
{
    APP.spm.stack[APP.spm.n_stack++] = marena_p(&APP.spm.m);
}

void spm_pop()
{
    assert(0 < APP.spm.n_stack);
    void *p = APP.spm.stack[--APP.spm.n_stack];
    marena_reset(&APP.spm.m, p);
}

void spm_align(usize alignment)
{
    marena_align(&APP.spm.m, alignment);
}

void *spm_alloc(usize s)
{
    void *mem = marena_alloc(&APP.spm.m, s);
#if PLTF_DEV_ENV
    usize rem = marena_rem(&APP.spm.m);
    if (rem < APP.spm.lowestleft) {
        pltf_log("SPM: %u kb\n", (u32)rem / 1024);
        APP.spm.lowestleft = rem;
    }
#endif
    assert(mem);
    return mem;
}

void *spm_allocz(usize s)
{
    void *mem = spm_alloc(s);
    if (mem) {
        mclr(mem, s);
    }
    return mem;
}

void *spm_alloc_aligned(usize s, usize alignment)
{
    void *mem = marena_alloc_aligned(&APP.spm.m, s, alignment);
#if PLTF_DEV_ENV
    usize rem = marena_rem(&APP.spm.m);
    if (rem < APP.spm.lowestleft) {
        pltf_log("SPM: %u kb\n", (u32)rem / 1024);
        APP.spm.lowestleft = rem;
    }
#endif
    return mem;
}

void *spm_allocz_aligned(usize s, usize alignment)
{
    void *mem = spm_alloc_aligned(s, alignment);
    if (mem) {
        mclr(mem, s);
    }
    return mem;
}

void *spm_alloc_rem(usize *s)
{
    return marena_alloc_rem(&APP.spm.m, s);
}

void spm_reset(void *p)
{
    marena_reset(&APP.spm.m, p);
    if (!p) {
        APP.spm.n_stack = 0;
    }
}

void *spm_alloc_aligned_ctx(void *ctx, usize s, usize alignment)
{
    return spm_alloc_aligned(s, alignment);
}

allocator_s spm_allocator()
{
    allocator_s a = {spm_alloc_aligned_ctx, 0};
    return a;
}