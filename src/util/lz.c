// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// compression, especially used for textures and level data
// LZ4 algorithm

#include "util/lz.h"
#include "pltf/pltf.h"
#include "util/bitrw.h"
#include "util/mathfunc.h"

#define LZ4_MAX_OFF  65535
#define LZ4_CPY_MIN  4
#define LZ4_FILE_BUF 4096

typedef struct lz_header_s {
    u32 size; // size of uncompressed data
} lz_header_s;

usize lz_decoded_size_file(void *f)
{
    lz_header_s head;
    i32         p = pltf_file_tell(f);
    pltf_file_r(f, &head, sizeof(lz_header_s));
    pltf_file_seek_set(f, p);
    return head.size;
}

usize lz_decoded_size(const void *src)
{
    const lz_header_s *head = (const lz_header_s *)src;
    return head->size;
}

static i32 lz_decode_file_r_byte(void *f, byte *buffer, byte **p)
{
    // it doesn't really matter if we read past the source bytes
    // bytes are not read, and EOF is detected anyway

    if (*p == (buffer + LZ4_FILE_BUF)) {
        *p = buffer;
        pltf_file_r(f, buffer, LZ4_FILE_BUF);
    }
    i32 b = *(u8 *)(*p)++;
    return b;
}

static i32 lz4_decode_file_runl(void *f, byte *buffer, byte **p)
{
    i32 l = 0;
    while (1) {
        i32 b = lz_decode_file_r_byte(f, buffer, p);
        l += b;
        if (b < 255) break;
    }
    return l;
}

usize lz4_decode_file(void *f, void *dst)
{
    lz_header_s h;
    if (!pltf_file_r_checked(f, &h, sizeof(lz_header_s))) return 0;

    ALIGNAS(32) byte buffer[LZ4_FILE_BUF];

    byte *p     = &buffer[LZ4_FILE_BUF]; // force buffer fill on first read
    byte *d     = (byte *)dst;
    byte *d_end = (byte *)dst + h.size;

    while (1) {
        i32 tok = lz_decode_file_r_byte(f, buffer, &p);

        i32 n_lit = tok >> 4;
        if (n_lit == 15) {
            n_lit += lz4_decode_file_runl(f, buffer, &p);
        }

        byte *d_end_lit = d + n_lit;
        while (d < d_end_lit) {
            *d++ = lz_decode_file_r_byte(f, buffer, &p);
        }

        if (d == d_end) {
            break;
        }

        i32 n_cpy = tok & 15;
        i32 offs  = 0;
        offs |= lz_decode_file_r_byte(f, buffer, &p);
        offs |= lz_decode_file_r_byte(f, buffer, &p) << 8;

        if (n_cpy == 15) {
            n_cpy += lz4_decode_file_runl(f, buffer, &p);
        }

        byte *d_end_cpy = d + n_cpy + 4;
        do {
            *d = *(d - offs);
            d++;
        } while (d < d_end_cpy);
    }
    assert((usize)(d - (byte *)dst) == h.size);
    return (usize)(d - (byte *)dst);
}

static i32 lz4_decode_runl(byte **pp)
{
    i32 l = 0;
    while (1) {
        i32 b = *(u8 *)(*pp)++;
        l += b;
        if (b < 255) break;
    }
    return l;
}

usize lz4_decode(const void *src, void *dst)
{
    lz_header_s *head = (lz_header_s *)src;

    byte *s     = (byte *)src + sizeof(lz_header_s);
    byte *d     = (byte *)dst;
    byte *d_end = (byte *)dst + head->size;

    while (1) {
        i32 tok = *(u8 *)s++;

        i32 n_lit = tok >> 4;
        if (n_lit == 15) {
            n_lit += lz4_decode_runl(&s);
        }

        byte *d_end_lit = d + n_lit;
        while (d < d_end_lit) {
            *d++ = *s++;
        }

        if (d == d_end)
            break;

        i32 n_cpy = tok & 15;
        i32 offs  = 0;
        offs |= (i32)(*(u8 *)s++);
        offs |= (i32)(*(u8 *)s++) << 8;

        if (n_cpy == 15) {
            n_cpy += lz4_decode_runl(&s);
        }

        byte *d_end_cpy = d + n_cpy + 4;
        do {
            *d = *(d - offs);
            d++;
        } while (d < d_end_cpy);
    }
    assert((usize)(d - (byte *)dst) == head->size);
    return (usize)(d - (byte *)dst);
}