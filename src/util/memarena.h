// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MEMARENA_H
#define MEMARENA_H

#include "mem.h"

typedef struct memarena_s {
    void *buf_og;
    void *buf;
    usize bufsize;
    usize rem;
    byte *p;
} marena_s;

void  memarena_init(marena_s *m, void *buf, usize bufsize);
void  memarena_align(marena_s *m, usize alignment);
void *memarena_alloc(marena_s *m, usize s);
void *memarena_alloc_aligned(marena_s *m, usize s, usize alignment);
void *memarena_state(marena_s *m);
void  memarena_reset_to(marena_s *m, void *p);
void  memarena_reset(marena_s *m);
void *memarena_alloc_rem(marena_s *m, usize *s);
usize memarena_size_rem(marena_s *m);

#endif