// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef LZSS_H
#define LZSS_H

#include "pltf/pltf_types.h"

usize lzss_decoded_size(const void *src);
usize lzss_decode_ext(const void *src, void *dst, i32 bits_run);
usize lzss_decode(const void *src, void *dst);
usize lzss_encode(const void *src, usize srcl, void *dst);
//
usize lzss_decode_file_peek_size(void *f);
usize lzss_decode_file(void *f, void *dst);

#endif