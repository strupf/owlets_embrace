#ifndef OS_MEM_H
#define OS_MEM_H

#include "os_types.h"

// MEMORY REPLACEMENTS =========================================================
static inline void c_memset(void *dst, int val, size_t l)
{
        int   len = (int)l;
        char *d   = (char *)dst;
        for (int n = 0; n < len; n++) *d++ = (char)val;
}

static inline void *c_memcpy(void *dst, const void *src, size_t l)
{
        if (dst == src) return dst;
        char       *d   = (char *)dst;
        const char *s   = (const char *)src;
        int         len = (int)l;
        for (int n = 0; n < len; n++) *d++ = *s++;
        return dst;
}

static inline void *c_memmove(void *dst, const void *src, size_t l)
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

static inline void c_memclr(void *dst, size_t l)
{
        int   len = (int)l;
        char *d   = (char *)dst;
        for (int n = 0; n < len; n++) d[n] = 0;
}

static inline void c_memclr4(void *dst, size_t l)
{
        c_assert(((uintptr_t)dst & 3) == 0);
        c_assert((l & 3) == 0);
        c_assert(sizeof(int) == 4);

        int *d   = (int *)dst;
        int  len = (int)l;
        for (int n = 0; n < len; n += 4) *d++ = 0;
}

static inline void *c_memcpy4(void *dst, const void *src, size_t l)
{
        if (dst == src) return dst;
        c_assert(((uintptr_t)dst & 3) == 0);
        c_assert(((uintptr_t)src & 3) == 0);
        c_assert((l & 3) == 0);
        c_assert(sizeof(int) == 4);

        int       *d   = (int *)dst;
        const int *s   = (const int *)src;
        int        len = (int)l;
        for (int n = 0; n < len; n += 4) *d++ = *s++;
        return dst;
}

#endif