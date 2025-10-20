// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef LZSS_H
#define LZSS_H

#include "pltf/pltf_types.h"

#define lz_decode      lz4_decode
#define lz_decode_file lz4_decode_file

usize lz_decoded_size_file(void *f);
usize lz_decoded_size(const void *src);

usize lz4_decode(const void *src, void *dst);
usize lz4_decode_file(void *f, void *dst);

#endif