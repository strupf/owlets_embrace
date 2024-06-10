// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "mem.h"

void *alignup_ptr(void *p)
{
    uptr pa = ((uptr)p + (uptr)(PLTF_SIZE_CL - 1)) & ~(uptr)(PLTF_SIZE_CL - 1);
    return (void *)pa;
}

void *aligndn_ptr(void *p)
{
    return (void *)((uptr)p & ~(uptr)(PLTF_SIZE_CL - 1));
}

usize alignup_usize(usize p)
{
    return ((p + (usize)(PLTF_SIZE_CL - 1)) & ~(usize)(PLTF_SIZE_CL - 1));
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

// MARENA ======================================================================

void marena_init(marena_s *m, void *buf, usize bufsize)
{
    mspan_s sp = {buf, bufsize};
    sp         = mspan_align(sp);
    m->buf_og  = buf;
    m->buf     = sp.p;
    m->bufsize = sp.size;
    marena_reset(m);
}

void *marena_alloc(marena_s *m, usize s)
{
    usize size = alignup_usize(s);
    if (m->rem < size) return NULL;
    void *mem = m->p;
    m->p += size;
    m->rem -= size;
    return mem;
}

void *marena_state(marena_s *m)
{
    return m->p;
}

void marena_reset_to(marena_s *m, void *p)
{
    m->p       = (char *)p;
    usize offs = ((char *)p - (char *)m->buf);
    m->rem     = m->bufsize - offs;
}

void marena_reset(marena_s *m)
{
    m->p   = (char *)m->buf;
    m->rem = m->bufsize;
}

void *marena_alloc_rem(marena_s *m, usize *s)
{
    if (s) *s = m->rem;
    void *mem = m->p;
    m->p += m->rem;
    m->rem = 0;
    return mem;
}

usize marena_size_rem(marena_s *m)
{
    return m->rem;
}

// MHEAP =======================================================================

static bool32     mhblock_is_busy(mhblock_s *b);
static usize      mhblock_size_needed(usize s);
static mhblock_s *mhblock_aquire_free(mheap_s *m, usize size);
static void       mhblock_add_to(mhblock_s **head, mhblock_s *b);
static void       mhblock_remove_from(mhblock_s **head, mhblock_s *b);

void mheap_init(mheap_s *m, void *buf, usize bufsize)
{
    mspan_s sp = {buf, bufsize};
    sp         = mspan_align(sp);
    m->buf_og  = buf;
    m->buf     = sp.p;
    m->bufsize = sp.size;
    mheap_reset(m);
}

void mheap_reset(mheap_s *m)
{
    mhblock_s *b = (mhblock_s *)m->buf;
    b->next      = NULL;
    b->prev      = NULL;
    b->nextphys  = NULL;
    b->prevphys  = NULL;
    b->s         = m->bufsize;
    m->free      = b;
    m->busy      = NULL;
}

void *mheap_alloc(mheap_s *m, usize s)
{
    usize      size = mhblock_size_needed(s);
    mhblock_s *b    = mhblock_aquire_free(m, size);
    mhblock_add_to(&m->busy, b);

    b->s |= 1U; // mark as busy
    void *user = (void *)(b + 1);
    mheap_check(m);
    return user;
}

void mheap_free(mheap_s *m, void *ptr)
{
    mhblock_s *b = (mhblock_s *)ptr - 1;
    mhblock_remove_from(&m->busy, b);
    b->s &= ~1U; // clear busy flag
    mhblock_s *p = b->prevphys;
    mhblock_s *n = b->nextphys;

    if (p && !mhblock_is_busy(p)) { // try merge left block
        mhblock_remove_from(&m->free, p);
        p->s += b->s;
        p->nextphys = n;
        if (n) {
            n->prevphys = p;
        }
        b = p;
    }

    if (n && !mhblock_is_busy(n)) { // try merge right block
        mhblock_remove_from(&m->free, n);
        b->s += n->s;
        b->nextphys = n->nextphys;
        if (b->nextphys) {
            b->nextphys->prevphys = b;
        }
    }

    mhblock_add_to(&m->free, b);
    mheap_check(m);
}

void *mheap_realloc(mheap_s *m, void *ptr, usize s)
{
    usize      size = mhblock_size_needed(s);
    mhblock_s *b    = (mhblock_s *)ptr - 1;
    if (b->s >= size) return ptr;

    void *user = mheap_alloc(m, s);
    if (!user) return NULL;

    mcpy(user, ptr, b->s - sizeof(mhblock_s));
    mheap_free(m, ptr);
    mheap_check(m);
    return user;
}

static bool32 mhblock_is_busy(mhblock_s *b)
{
    return (b->s & 1);
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

static mhblock_s *mhblock_aquire_free(mheap_s *m, usize size)
{
    for (mhblock_s *b = m->free; b; b = b->next) {
        assert(!mhblock_is_busy(b));
        if (b->s < size) continue;
        mhblock_remove_from(&m->free, b);
        usize exc = (b->s - size);
        if (exc >= 2 * sizeof(mhblock_s)) {
            mhblock_s *s = (mhblock_s *)((char *)b + size);
            s->prev      = NULL;
            s->next      = NULL;
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
    BAD_PATH
    return NULL;
}

static usize mhblock_size_needed(usize s)
{
    return sizeof(mhblock_s) + alignup_usize(s);
}

#ifdef SYS_DEBUG
static usize mhblock_size(mhblock_s *b)
{
    return (b->s & ~(usize)1);
}

static int mhblock_loc(mheap_s *m, mhblock_s *b)
{
    return b ? (int)((char *)b - (char *)m->buf) : -1;
}

void mheap_print(mheap_s *m)
{
    pltf_log("\n");
    for (mhblock_s *b = (mhblock_s *)m->buf; b; b = b->nextphys) {
        pltf_log("%i (%i - %i) | pneigh %i | nneigh %i | n %i | p %i\n",
                 mhblock_loc(m, b),
                 (int)mhblock_size(b),
                 mhblock_is_busy(b),
                 mhblock_loc(m, b->prevphys),
                 mhblock_loc(m, b->nextphys),
                 mhblock_loc(m, b->prev),
                 mhblock_loc(m, b->next));
    }
}

void mheap_check(mheap_s *m)
{
    for (mhblock_s *b = (mhblock_s *)m->buf; b; b = b->nextphys) {
        if (b->nextphys) {
            assert(b->nextphys->prevphys == b);
            assert((mhblock_s *)((char *)b + mhblock_size(b)) == b->nextphys);
        }
        if (b->prevphys) {
            assert(b->prevphys->nextphys == b);
        }
        if (b->next) {
            assert(b->next->prev == b);
        }
        if (b->prev) {
            assert(b->prev->next == b);
        }
    }
}
#endif