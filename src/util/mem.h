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

#define alignup_ptr(PTR) align_ptr(PTR, 32)
// returns a pointer aligned to the specified alignment
void *align_ptr(void *p, usize alignment);
// rounds up to the next multiple of word size
#define alignup_usize(S) align_usize(S, 32)
usize   align_usize(usize p, usize alignment);
// rounds up to the prev multiple of word size
usize   aligndn_usize(usize p);
// places the span beginning at the next word address; size gets adjusted
mspan_s mspan_align(mspan_s m);
//

#endif