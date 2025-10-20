// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/spm.h"
#include "app.h"

spm_s g_SPM;

void spm_init()
{
    marena_init(&g_SPM.m, g_SPM.mem, sizeof(g_SPM.mem));
    pltf_log("SPM init with: %u kb\n", (u32)sizeof(g_SPM.mem) / 1024);
#if PLTF_DEV_ENV
    g_SPM.lowestleft = sizeof(g_SPM.mem);
#endif
}

void spm_push()
{
    g_SPM.stack[g_SPM.n_stack++] = marena_p(&g_SPM.m);
}

void spm_pop()
{
    assert(0 < g_SPM.n_stack);
    void *p = g_SPM.stack[--g_SPM.n_stack];
    marena_reset(&g_SPM.m, p);
}

void spm_align(usize alignment)
{
    marena_align(&g_SPM.m, alignment);
}

void *spm_alloc(usize s)
{
    void *mem = marena_alloc(&g_SPM.m, s);
#if PLTF_DEV_ENV
    usize rem = marena_rem(&g_SPM.m);
    if (rem < g_SPM.lowestleft) {
        pltf_log("SPM: %u kb\n", (u32)rem / 1024);
        g_SPM.lowestleft = rem;
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
    void *mem = marena_alloc_aligned(&g_SPM.m, s, alignment);
#if PLTF_DEV_ENV
    usize rem = marena_rem(&g_SPM.m);
    if (rem < g_SPM.lowestleft) {
        pltf_log("SPM: %u kb\n", (u32)rem / 1024);
        g_SPM.lowestleft = rem;
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
    return marena_alloc_rem(&g_SPM.m, s);
}

void spm_reset(void *p)
{
    marena_reset(&g_SPM.m, p);
    if (!p) {
        g_SPM.n_stack = 0;
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