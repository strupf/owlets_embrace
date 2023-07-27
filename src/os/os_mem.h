/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#ifndef OS_MEM_H
#define OS_MEM_H

#include "os_types.h"

typedef union {
        char  byte[1024];
        void *align;
} kilobyte_s;

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

// internal scratchpad memory stack
// just a fixed sized bump allocator
void  os_spmem_push();                   // push the current state
void  os_spmem_pop();                    // pop and restore previous state
void  os_spmem_clr();                    // reset bump allocator
void *os_spmem_alloc(size_t size);       // allocate memory
void *os_spmem_alloc_rems(size_t *size); // allocate remaining memory
void *os_spmem_allocz(size_t size);      // allocate and zero memory
void *os_spmem_allocz_rem(size_t *size); // allocate and zero remaining memory

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