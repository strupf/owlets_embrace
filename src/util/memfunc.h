// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os/os_types.h"

typedef struct memheapblock_s memheapblock_s;
struct memheapblock_s {
        size_t          s;
        memheapblock_s *next;
        memheapblock_s *prev;
};

typedef struct {
        void           *buf;
        size_t          bufsize;
        memheapblock_s *free;
        memheapblock_s *busy;
} memheap_s;

void  memheap_init(memheap_s *m, void *buf, size_t bufsize);
void *memheap_alloc(memheap_s *m, size_t s);
void  memheap_free(memheap_s *m, void *p);
void *memheap_realloc(memheap_s *m, void *p, size_t s);
void  memheap_clr(memheap_s *m);
