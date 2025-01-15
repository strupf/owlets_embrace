// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "mem.h"
#include "mathfunc.h"

void *align_ptr(void *p, usize alignment)
{
    if (!alignment) {
        uptr pa = ((uptr)p + alignment - 1) & ~(alignment - 1);
        return (void *)pa;
    } else {
        return p;
    }
}

usize align_usize(usize s, usize alignment)
{
    return (s + alignment - 1) & ~(alignment - 1);
}

mspan_s mspan_align(mspan_s m, usize alignment)
{
    void   *p_beg = align_ptr(m.p, alignment);
    void   *p_end = (byte *)m.p + m.size;
    mspan_s r     = {p_beg, (usize)((byte *)p_end - (byte *)p_beg)};
    return r;
}

u32 checksum_u32(const void *p, usize size)
{
    const u8 *d = (const u8 *)p;
    usize     l = size;
    u32       s = 0;
    u32       r = 0;
    while (l) {
        r += (u32)*d++ << s;
        l--;
        s = (s + 8) & 31;
    }
    return r;
}