// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MARENA_H
#define MARENA_H

#include "mem.h"

typedef struct marena_s {
    byte *p;
    byte *buf;
    usize bufsize;
} marena_s;

void  marena_init(marena_s *m, void *buf, usize bufs);
void  marena_init_aligned(marena_s *m, void *buf, usize bufs, usize alignment);
void  marena_align(marena_s *m, usize alignment);
void *marena_alloc(marena_s *m, usize s);
void *marena_alloc_aligned(marena_s *m, usize s, usize alignment);
void *marena_p(marena_s *m);
void  marena_reset(marena_s *m, void *p);
void *marena_alloc_rem(marena_s *m, usize *s);
usize marena_rem(marena_s *m);

#endif