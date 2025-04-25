// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// write and read bits to a buffer

#ifndef BITRW_H
#define BITRW_H

#include "pltf/pltf_types.h"

typedef struct {
    u32 *b;
    i32  pos; // [0, 32]
} bitrw_s;

// read up to 32 bits from the buffer
u32 bitrw_r(bitrw_s *br, i32 nbits);

// write up to 32 bits to the buffer
void bitrw_w(bitrw_s *bw, u32 x, i32 nbits);

// read 1 bit from the buffer
i32 bitrw_r1(bitrw_s *br);

// write 1 bit to the buffer
void bitrw_w1(bitrw_s *bw, i32 x);
#endif