// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "spm.h"
#include "app.h"

static void  *spm_alloc_ctx(void *ctx, usize s);
const alloc_s spm_allocator = {spm_alloc_ctx, 0};

static void *spm_alloc_ctx(void *ctx, usize s)
{
    return spm_alloc(s);
}

void spm_init()
{
    marena_init(&APP->spm.m, APP->spm.mem, sizeof(APP->spm.mem));
#if PLTF_DEV_ENV
    APP->spm.lowestleft = sizeof(APP->spm.mem);
#endif
}

void spm_push()
{
    APP->spm.stack[APP->spm.n_stack++] = marena_p(&APP->spm.m);
}

void spm_pop()
{
    assert(0 < APP->spm.n_stack);
    void *p = APP->spm.stack[--APP->spm.n_stack];
    marena_reset(&APP->spm.m, p);
}

void spm_align(usize alignment)
{
    marena_align(&APP->spm.m, alignment);
}

void *spm_alloc(usize s)
{
    void *mem = marena_alloc(&APP->spm.m, s);
#if PLTF_DEV_ENV
    usize rem = marena_rem(&APP->spm.m);
    if (rem < APP->spm.lowestleft) {
        pltf_log("SPM: %u kb\n", (u32)rem / 1024);
        APP->spm.lowestleft = rem;
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
    void *mem = marena_alloc_aligned(&APP->spm.m, s, alignment);
#if PLTF_DEV_ENV
    usize rem = marena_rem(&APP->spm.m);
    if (rem < APP->spm.lowestleft) {
        pltf_log("SPM: %u kb\n", (u32)rem / 1024);
        APP->spm.lowestleft = rem;
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
    return marena_alloc_rem(&APP->spm.m, s);
}

void spm_reset(void *p)
{
    marena_reset(&APP->spm.m, p);
    if (!p) {
        APP->spm.n_stack = 0;
    }
}

void *spm_alloc_aligned_ctx(void *ctx, usize s, usize alignment)
{
    return spm_alloc_aligned(s, alignment);
}

allocator_s spm_allocator2()
{
    allocator_s a = {spm_alloc_aligned_ctx, 0};
    return a;
}