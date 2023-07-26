#ifndef OS_MEM_H
#define OS_MEM_H

#include "os_types.h"

// MEMORY REPLACEMENTS =========================================================
static inline void os_memset(void *dst, int val, size_t l)
{
        int   len = (int)l;
        char *d   = (char *)dst;
        for (int n = 0; n < len; n++) *d++ = (char)val;
}

static inline void *os_memcpy(void *dst, const void *src, size_t l)
{
        if (dst == src) return dst;
        char       *d   = (char *)dst;
        const char *s   = (const char *)src;
        int         len = (int)l;
        for (int n = 0; n < len; n++) *d++ = *s++;
        return dst;
}

static inline void *os_memmov(void *dst, const void *src, size_t l)
{
        if (dst == src) return dst;
        char       *d   = (char *)dst;
        const char *s   = (const char *)src;
        int         len = (int)l;
        if (d < s)
                for (int n = 0; n < len; n++) *d++ = *s++;
        else
                for (int n = len - 1; n >= 0; n--) *d-- = *s--;
        return dst;
}

static inline void os_memclr(void *dst, size_t l)
{
        int   len = (int)l;
        char *d   = (char *)dst;
        for (int n = 0; n < len; n++) d[n] = 0;
}

static inline void os_memclr4(void *dst, size_t l)
{
        ASSERT(((uintptr_t)dst & 3) == 0);
        ASSERT((l & 3) == 0);
        ASSERT(sizeof(int) == 4);

        int *d   = (int *)dst;
        int  len = (int)l;
        for (int n = 0; n < len; n += 4) *d++ = 0;
}

static inline void *os_memcpy4(void *dst, const void *src, size_t l)
{
        if (dst == src) return dst;
        ASSERT(((uintptr_t)dst & 3) == 0);
        ASSERT(((uintptr_t)src & 3) == 0);
        ASSERT((l & 3) == 0);
        ASSERT(sizeof(int) == 4);

        int       *d   = (int *)dst;
        const int *s   = (const int *)src;
        int        len = (int)l;
        for (int n = 0; n < len; n += 4) *d++ = *s++;
        return dst;
}

typedef struct {
        char *p;
        char *pr;
        char *mem;
} memarena_s;

static inline void memarena_init(memarena_s *m, void *buf, size_t bufsize)
{
        ASSERT(m && buf && bufsize);
        m->mem = (char *)buf;
        m->p   = (char *)buf;
        m->pr  = (char *)buf + bufsize;
}

static inline void *memarena_alloc(memarena_s *m, size_t s)
{
        ASSERT(m && m->mem);
        size_t size = (s + 3u) & ~3u;
        int    dp   = m->pr - m->p;
        ASSERT(dp >= (int)size);
        void *mem = m->p;
        m->p += size;
        return mem;
}

static inline void *memarena_alloc_rem(memarena_s *m, size_t *s)
{
        ASSERT(m && m->mem && s && m->p < m->pr);
        *s        = m->pr - m->p;
        void *mem = m->p;
        m->p      = m->pr;
        return mem;
}

static inline void *memarena_allocz(memarena_s *m, size_t s)
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
static inline void *memarena_allocz_rem(memarena_s *m, size_t *s)
{
        ASSERT(m && m->mem && s);
        void *mem = memarena_alloc_rem(m, s);
        ASSERT(((*s) & 3) == 0);
        os_memclr4(mem, *s);
        return mem;
}

static inline void *memarena_peek(memarena_s *m)
{
        ASSERT(m && m->mem);
        return m->p;
}

static inline void memarena_set(memarena_s *m, void *p)
{
        ASSERT(m && m->mem && m->mem <= (char *)p && (char *)p <= m->pr);
        m->p = (char *)p;
}

static inline void memarena_clr(memarena_s *m)
{
        ASSERT(m && m->mem);
        m->p = (char *)m->mem;
}

#endif