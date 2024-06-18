// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MEM_H
#define MEM_H

#include "pltf/pltf.h"

#define MKILOBYTE(X) ((X)*1024)
#define MMEGABYTE(X) ((X)*1024 * 1024)

typedef union {
    char  b[0x400];
    void *align_min;
} mkilobyte_s;

typedef union {
    char  b[0x100000];
    void *align_min;
} mmegabyte_s;

typedef struct {
    void *p;
    u32 size;
} mspan_s;

typedef struct {
    void *buf_og;
    void *buf;
    u32 bufsize;
    u32 rem;
    char *p;
} marena_s;

typedef struct mhblock_s mhblock_s;
struct mhblock_s {
    ALIGNCL
    mhblock_s *next;
    mhblock_s *prev;
    mhblock_s *nextphys;
    mhblock_s *prevphys;
    u32      s;
};

typedef struct {
    void      *buf_og;
    void      *buf;
    u32      bufsize;
    mhblock_s *free;
    mhblock_s *busy;
} mheap_s;

// returns a pointer aligned to the next word address
void   *alignup_ptr(void *p);
// returns a pointer aligned to the prev word address
void   *aligndn_ptr(void *p);
// rounds up to the next multiple of word size
u32   alignup_usize(u32 p);
// rounds up to the prev multiple of word size
u32   aligndn_usize(u32 p);
// places the span beginning at the next word address; size gets adjusted
mspan_s mspan_align(mspan_s m);
//
void    marena_init(marena_s *m, void *buf, u32 bufsize);
void   *marena_alloc(marena_s *m, u32 s);
void   *marena_state(marena_s *m);
void    marena_reset_to(marena_s *m, void *p);
void    marena_reset(marena_s *m);
void   *marena_alloc_rem(marena_s *m, u32 *s);
u32   marena_size_rem(marena_s *m);
//
void    mheap_init(mheap_s *m, void *buf, u32 bufsize);
void    mheap_reset(mheap_s *m);
void   *mheap_alloc(mheap_s *m, u32 s);
void    mheap_free(mheap_s *m, void *p);
void   *mheap_realloc(mheap_s *m, void *p, u32 s);
#ifdef SYS_DEBUG
void mheap_check(mheap_s *m);
void mheap_print(mheap_s *m);
#else
#define mheap_check(X)
#define mheap_print(X)
#endif
#endif