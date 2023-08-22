// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "memfunc.h"

void memheap_init(memheap_s *m, void *buf, size_t bufsize)
{
        m->buf     = buf;
        m->bufsize = bufsize;
        memheap_clr(m);
}

void *memheap_alloc(memheap_s *m, size_t s)
{
        size_t size = sizeof(memheapblock_s) + s;
        NOT_IMPLEMENTED
        for (memheapblock_s *b = m->free, *p = NULL; b; p = b, b = b->next) {
                if (b->s < size) continue;

                size_t excess = (b->s - size);
                if (excess >= 64) {
                        memheapblock_s *t;
                }

                if (p) {
                }

                void *usermem = (void *)(b + 1);
                return usermem;
        }

        return NULL;
}

void memheap_free(memheap_s *m, void *p)
{
        memheapblock_s *blockp = (memheapblock_s *)p - 1;

        for (memheapblock_s *b = m->free, *p = NULL; b; p = b, b = b->next) {
        }
}

void *memheap_realloc(memheap_s *m, void *p, size_t s)
{
        return NULL;
}

void memheap_clr(memheap_s *m)
{
        m->busy       = NULL;
        m->free       = (memheapblock_s *)m->buf;
        m->free->s    = m->bufsize;
        m->free->next = NULL;
        m->free->prev = NULL;
}
