// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "memarena.h"

void memarena_init(marena_s *m, void *buf, usize bufsize)
{
    mspan_s sp = {buf, bufsize};
    sp         = mspan_align(sp);
    m->buf_og  = buf;
    m->buf     = sp.p;
    m->bufsize = sp.size;
    memarena_reset(m);
}

void memarena_align(marena_s *m, usize alignment)
{
    uptr  p_aligned = ((uptr)m->p + (alignment - 1)) & ~(uptr)(alignment - 1);
    usize dt        = (usize)((byte *)p_aligned - (byte *)m->p);
    m->p            = (byte *)p_aligned;
    m->rem -= dt;
}

void *memarena_alloc(marena_s *m, usize s)
{
    if (m->rem < s) return 0;
    void *mem = m->p;
    m->p += s;
    m->rem -= s;
    return mem;
}

void *memarena_alloc_aligned(marena_s *m, usize s, usize alignment)
{
    void *p = memarena_alloc(m, s + alignment);
    return (p ? align_ptr(p, alignment) : 0);
}

void *memarena_state(marena_s *m)
{
    return m->p;
}

void memarena_reset_to(marena_s *m, void *p)
{
    m->p       = (byte *)p;
    usize offs = (usize)((byte *)p - (byte *)m->buf);
    m->rem     = m->bufsize - offs;
}

void memarena_reset(marena_s *m)
{
    m->p   = (byte *)m->buf;
    m->rem = m->bufsize;
}

void *memarena_alloc_rem(marena_s *m, usize *s)
{
    if (s) *s = m->rem;
    void *mem = m->p;
    m->p += m->rem;
    m->rem = 0;
    return mem;
}

usize memarena_size_rem(marena_s *m)
{
    return m->rem;
}