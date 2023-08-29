// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MEMFUNC_H
#define MEMFUNC_H

#include "os/os_types.h"

typedef struct mhblock_s mhblock_s;
struct mhblock_s {
        size_t     s;
        mhblock_s *next;
        mhblock_s *prev;
        mhblock_s *nextphys;
        mhblock_s *prevphys;
};

typedef struct {
        void      *buf;
        size_t     bufsize;
        mhblock_s *free;
        mhblock_s *busy;
} memheap_s;

static inline size_t memalign_to_word(size_t s)
{
        return ((s + 3) & ~(size_t)3);
}

void  memheap_init(memheap_s *m, void *buf, size_t bufsize);
void *memheap_alloc(memheap_s *m, size_t s);
void *memheap_allocz(memheap_s *m, size_t s);
void  memheap_free(memheap_s *m, void *ptr);
void *memheap_realloc(memheap_s *m, void *ptr, size_t s);
void  memheap_clr(memheap_s *m);

void memheap_print(memheap_s *m);

typedef struct {
        char *p;
        char *pr;
        char *mem;
} memarena_s;

void  memarena_init(memarena_s *m, void *buf, size_t bufsize);
void *memarena_alloc(memarena_s *m, size_t s);
void *memarena_alloc_rem(memarena_s *m, size_t *s);
void *memarena_allocz(memarena_s *m, size_t s);
void *memarena_allocz_rem(memarena_s *m, size_t *s);
void *memarena_peek(memarena_s *m);
void  memarena_set(memarena_s *m, void *p);
void  memarena_clr(memarena_s *m);
#endif