// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "memfunc.h"
#include "os/os.h"

static inline bool32 mhblock_busy(mhblock_s *b)
{
        return (b->s & 1);
}

static inline size_t mhblock_size(mhblock_s *b)
{
        return (b->s & ~(size_t)1);
}

static void memheap_check(memheap_s *m)
{
        for (mhblock_s *b = (mhblock_s *)m->buf; b; b = b->nextphys) {
                if (b->nextphys) {
                        ASSERT(b->nextphys->prevphys == b);
                        ASSERT((mhblock_s *)((char *)b + mhblock_size(b)) == b->nextphys);
                }
                if (b->prevphys) {
                        ASSERT(b->prevphys->nextphys == b);
                }
                if (b->next) {
                        ASSERT(b->next->prev == b);
                }
                if (b->prev) {
                        ASSERT(b->prev->next == b);
                }
        }
}

void memheap_init(memheap_s *m, void *buf, size_t bufsize)
{
        ASSERT(m && buf && bufsize > sizeof(mhblock_s) * 2);
        ASSERT((sizeof(mhblock_s) & 3) == 0);
        int    aligner = (4 - ((uintptr_t)buf & 3)) & 3;
        size_t bufs    = bufsize - ((char *)m->buf - (char *)buf);
        m->buf         = (char *)buf + aligner;
        m->bufsize     = bufsize - aligner;
        memheap_clr(m);
}

static void mhblock_remove_from(mhblock_s **head, mhblock_s *b)
{
        mhblock_s *n = b->next;
        mhblock_s *p = b->prev;
        if (n) n->prev = p;
        if (p) p->next = n;
        else *head = n;
        b->next = NULL;
        b->prev = NULL;
}

static void mhblock_add_to(mhblock_s **head, mhblock_s *b)
{
        if (*head) (*head)->prev = b;
        b->next = *head;
        *head   = b;
}

static mhblock_s *mhblock_aquire_free(memheap_s *m, size_t size)
{
        for (mhblock_s *b = m->free; b; b = b->next) {
                ASSERT(!mhblock_busy(b));
                if (b->s < size) continue;
                mhblock_remove_from(&m->free, b);
                size_t exc = (b->s - size);
                if (exc >= 2 * sizeof(mhblock_s)) {
                        mhblock_s *s = (mhblock_s *)((char *)b + size);
                        s->s         = exc;
                        b->s         = size;
                        s->prevphys  = b;
                        s->nextphys  = b->nextphys;
                        b->nextphys  = s;
                        if (s->nextphys) {
                                s->nextphys->prevphys = s;
                        }
                        mhblock_add_to(&m->free, s);
                }
                return b;
        }
        ASSERT(0);
        return NULL;
}

static inline size_t mhblock_size_needed(size_t s)
{
        return sizeof(mhblock_s) + memalign_to_word(s);
}

void *memheap_alloc(memheap_s *m, size_t s)
{
        size_t     size = mhblock_size_needed(s);
        mhblock_s *b    = mhblock_aquire_free(m, size);
        mhblock_add_to(&m->busy, b);

        b->s |= 1; // mark as busy
        void *user = (void *)(b + 1);
        memheap_check(m);
        return user;
}

void *memheap_allocz(memheap_s *m, size_t s)
{
        void *user = memheap_alloc(m, s);
        if (!user) return NULL;

        size_t size = memalign_to_word(s);
        os_memclr4(user, size);
        return user;
}

void memheap_free(memheap_s *m, void *ptr)
{
        mhblock_s *b = (mhblock_s *)ptr - 1;
        mhblock_remove_from(&m->busy, b);
        b->s &= ~1; // clear busy flag
        mhblock_s *p = b->prevphys;
        mhblock_s *n = b->nextphys;

        if (p && !mhblock_busy(p)) { // try merge left block
                mhblock_remove_from(&m->free, p);
                p->s += b->s;
                p->nextphys = n;
                if (n) {
                        n->prevphys = p;
                }
                b = p;
        }

        if (n && !mhblock_busy(n)) { // try merge right block
                mhblock_remove_from(&m->free, n);
                b->s += n->s;
                b->nextphys = n->nextphys;
                if (b->nextphys) {
                        b->nextphys->prevphys = b;
                }
        }

        mhblock_add_to(&m->free, b);
        memheap_check(m);
}

void *memheap_realloc(memheap_s *m, void *ptr, size_t s)
{
        size_t     size = mhblock_size_needed(s);
        mhblock_s *b    = (mhblock_s *)ptr - 1;
        if (b->s >= size) return ptr;

        void *user = memheap_alloc(m, s);
        if (!user) return NULL;

        os_memcpy4(user, ptr, memalign_to_word(b->s - sizeof(mhblock_s)));
        memheap_free(m, ptr);
        memheap_check(m);
        return user;
}

void memheap_clr(memheap_s *m)
{
        mhblock_s *b = (mhblock_s *)m->buf;
        *b           = (const mhblock_s){0};
        b->s         = m->bufsize;
        m->busy      = NULL;
        m->free      = b;
        memheap_check(m);
}

static inline int mhblock_loc(memheap_s *m, mhblock_s *b)
{
        return b ? (int)((char *)b - (char *)m->buf) : -1;
}

void memheap_print(memheap_s *m)
{
        PRINTF("\n");
        for (mhblock_s *b = (mhblock_s *)m->buf; b; b = b->nextphys) {
                PRINTF("%i (%i - %i) | pneigh %i | nneigh %i | n %i | p %i\n",
                       mhblock_loc(m, b),
                       (int)mhblock_size(b),
                       mhblock_busy(b),
                       mhblock_loc(m, b->prevphys),
                       mhblock_loc(m, b->nextphys),
                       mhblock_loc(m, b->prev),
                       mhblock_loc(m, b->next));
        }
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
        int    dp   = (int)(m->pr - m->p);
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
        int    dp   = (int)(m->pr - m->p);
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