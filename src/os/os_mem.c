// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os_internal.h"

void os_spmem_push()
{
        ASSERT(g_os.n_spmem < OS_SPMEM_STACK_HEIGHT);
        g_os.spmemstack[g_os.n_spmem++] = memarena_peek(&g_os.spmem);
}

void os_spmem_pop()
{
        ASSERT(g_os.n_spmem > 0);
        memarena_set(&g_os.spmem, g_os.spmemstack[--g_os.n_spmem]);
}

void *os_spmem_peek()
{
        return memarena_peek(&g_os.spmem);
}

void os_spmem_set(void *p)
{
        memarena_set(&g_os.spmem, p);
}

void os_spmem_clr()
{
        memarena_clr(&g_os.spmem);
        g_os.n_spmem = 0;
}

void *os_spmem_alloc(size_t size)
{
        return memarena_alloc(&g_os.spmem, size);
}

void *os_spmem_alloc_rems(size_t *size)
{
        return memarena_alloc_rem(&g_os.spmem, size);
}

void *os_spmem_allocz(size_t size)
{
        return memarena_allocz(&g_os.spmem, size);
}

void *os_spmem_allocz_rem(size_t *size)
{
        return memarena_allocz_rem(&g_os.spmem, size);
}

void memarena_init(memarena_s *m, void *buf, size_t bufsize)
{
        ASSERT(m && buf && bufsize);
        m->mem = (char *)buf;
        m->p   = (char *)buf;
        m->pr  = (char *)buf + bufsize;
}

void *memarena_alloc(memarena_s *m, size_t s)
{
        ASSERT(m && m->mem);
        size_t size = (s + 3u) & ~3u;
        int    dp   = m->pr - m->p;
        ASSERT(dp >= (int)size);
        void *mem = m->p;
        m->p += size;
        return mem;
}

void *memarena_alloc_rem(memarena_s *m, size_t *s)
{
        ASSERT(m && m->mem && s && m->p < m->pr);
        *s        = m->pr - m->p;
        void *mem = m->p;
        m->p      = m->pr;
        return mem;
}

void *memarena_allocz(memarena_s *m, size_t s)
{
        ASSERT(m && m->mem);
        size_t size = (s + 3u) & ~3u;
        int    dp   = m->pr - m->p;
        ASSERT(dp >= (int)size);
        void *mem = m->p;
        os_memclr4(mem, size);
        m->p += size;
        return mem;
}
void *memarena_allocz_rem(memarena_s *m, size_t *s)
{
        ASSERT(m && m->mem && s);
        void *mem = memarena_alloc_rem(m, s);
        ASSERT(((*s) & 3) == 0);
        os_memclr4(mem, *s);
        return mem;
}

void *memarena_peek(memarena_s *m)
{
        ASSERT(m && m->mem);
        return m->p;
}

void memarena_set(memarena_s *m, void *p)
{
        ASSERT(m && m->mem && m->mem <= (char *)p && (char *)p <= m->pr);
        m->p = (char *)p;
}

void memarena_clr(memarena_s *m)
{
        ASSERT(m && m->mem);
        m->p = (char *)m->mem;
}