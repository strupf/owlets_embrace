// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "marena.h"

void marena_init(marena_s *m, void *buf, usize bufs)
{
    marena_init_aligned(m, buf, bufs, 1);
}

void marena_init_aligned(marena_s *m, void *buf, usize bufs, usize alignment)
{
    void *p    = align_ptr(buf, 4);
    m->buf     = p;
    m->bufsize = bufs - (usize)((byte *)p - (byte *)buf);
    marena_reset(m, 0);
}

void marena_align(marena_s *m, usize alignment)
{
    m->p = (byte *)align_ptr(m->p, alignment);
}

void *marena_alloc(marena_s *m, usize s)
{
    if (marena_rem(m) < s) return 0;
    void *mem = m->p;
    m->p += s;
    return mem;
}

void *marena_alloc_aligned(marena_s *m, usize s, usize alignment)
{
    marena_align(m, alignment);
    return marena_alloc(m, s);
}

void *marena_p(marena_s *m)
{
    return m->p;
}

void marena_reset(marena_s *m, void *p)
{
    if (!p) {
        m->p = m->buf;
    } else if (m->buf <= (byte *)p && (byte *)p < m->buf + m->bufsize) {
        m->p = (byte *)p;
    }
}

void *marena_alloc_rem(marena_s *m, usize *s)
{
    if (s) {
        *s = marena_rem(m);
    }
    void *mem = m->p;
    m->p      = m->buf + m->bufsize;
    return mem;
}

usize marena_rem(marena_s *m)
{
    return (m->bufsize - (usize)(m->p - m->buf));
}