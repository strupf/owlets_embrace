// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MEM_H
#define MEM_H

#include "sys/sys_types.h"

#define MKILOBYTE(X) ((X)*1024)
#define MMEGABYTE(X) ((X)*1024 * 1024)

typedef struct {
    alignas(4) char byte[0x400];
} mkilobyte_s;

typedef struct {
    alignas(4) char byte[0x100000];
} mmegabyte_s;

typedef struct {
    void *p;
    usize size;
} mspan_s;

typedef struct {
    void *buf_og;
    void *buf;
    usize bufsize;
    usize rem;
    char *p;
} marena_s;

typedef struct mhblock_s mhblock_s;
struct mhblock_s {
    mhblock_s *next;
    mhblock_s *prev;
    mhblock_s *nextphys;
    mhblock_s *prevphys;
    usize      s;
};

static_assert((alignof(mhblock_s) & 3) == 0, "mhblock alignment");

typedef struct {
    void      *buf_og;
    void      *buf;
    usize      bufsize;
    mhblock_s *free;
    mhblock_s *busy;
} mheap_s;

typedef void *(*memalloc_f)(usize s);
typedef void (*memfree_f)(void *p);
typedef void *(*memrealloc_f)(void *p, usize s);

// returns a pointer aligned to the next word address
void   *alignup_ptr(void *p);
// returns a pointer aligned to the prev word address
void   *aligndn_ptr(void *p);
// rounds up to the next multiple of word size
usize   alignup_usize(usize p);
// rounds up to the prev multiple of word size
usize   aligndn_usize(usize p);
// places the span beginning at the next word address; size gets adjusted
mspan_s mspan_align(mspan_s m);
//
void    marena_init(marena_s *m, void *buf, usize bufsize);
void   *marena_alloc(marena_s *m, usize s);
void   *marena_state(marena_s *m);
void    marena_reset_to(marena_s *m, void *p);
void    marena_reset(marena_s *m);
void   *marena_alloc_rem(marena_s *m, usize *s);
usize   marena_size_rem(marena_s *m);
//
void    mheap_init(mheap_s *m, void *buf, usize bufsize);
void    mheap_reset(mheap_s *m);
void   *mheap_alloc(mheap_s *m, usize s);
void    mheap_free(mheap_s *m, void *p);
void   *mheap_realloc(mheap_s *m, void *p, usize s);
#ifdef SYS_DEBUG
void mheap_check(mheap_s *m);
void mheap_print(mheap_s *m);
#else
#define mheap_check(X)
#define mheap_print(X)
#endif
#endif