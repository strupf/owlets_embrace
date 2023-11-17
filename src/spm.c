// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "spm.h"

SPM_s SPM;

void spm_init()
{
    marena_init(&SPM.m, SPM.mem, sizeof(SPM.mem));
}

void spm_push()
{
    SPM.stack[SPM.n_stack++] = marena_state(&SPM.m);
}

void spm_pop()
{
    void *p = SPM.stack[--SPM.n_stack];
    marena_reset_to(&SPM.m, p);
}

void *spm_alloc(usize s)
{
    return marena_alloc(&SPM.m, s);
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