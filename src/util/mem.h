// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MEM_H
#define MEM_H

#include "pltf/pltf.h"

#define MKILOBYTE(X) ((X) * 1024)
#define MMEGABYTE(X) ((X) * 1024 * 1024)

typedef struct {
    void *p;
    usize size;
} mspan_s;

// aligning rounds up
void   *align_ptr(void *p, usize alignment);
usize   align_usize(usize s, usize alignment);
mspan_s mspan_align(mspan_s m, usize alignment);

u32 checksum_u32(const void *p, usize size);

#endif