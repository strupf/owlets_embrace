// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "mem.h"

void *align_ptr(void *p, usize alignment)
{
    uptr pa = ((uptr)p + alignment - 1) & ~(alignment - 1);
    return (void *)pa;
}

usize align_usize(usize p, usize alignment)
{
    return ((p + (usize)(alignment - 1)) & ~(usize)(alignment - 1));
}

usize aligndn_usize(usize p)
{
    return (p & ~(usize)(PLTF_SIZE_CL - 1));
}

mspan_s mspan_align(mspan_s m)
{
    void   *p0 = alignup_ptr(m.p);
    void   *p1 = (char *)m.p + m.size;
    usize   s  = (usize)((char *)p1 - (char *)p0);
    mspan_s r  = {(void *)p0, s};
    return r;
}