// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// compression, especially used for textures and level data
//
// contains a bitflag byte prior to multiple encodings
// bit in byte set: copy that byte as is from source to destination
// otherwise: run length encoded, look back n_offset bytes in destination buffer
// and copy n_run bytes from destination offset to destination
//
// uses 16 bit to encode offset and runlength for decoding simplicity

#include "util/lzss.h"
#include "pltf/pltf.h"
#include "util/bitrw.h"
#include "util/mathfunc.h"

typedef struct {
    u32 nbytes; // bytes of compressed data
    u32 size;   // size of uncompressed data
} lzss_header_s;

#define LZSS_NBITS_OFF     8
#define LZSS_NBITS_RUN     (16 - LZSS_NBITS_OFF)
#define LZSS_LIT_THRESHOLD 3
#define LZSS_MAX_OFF       (1 << LZSS_NBITS_OFF)
#define LZSS_MAX_RUN       (LZSS_LIT_THRESHOLD - 1 + (1 << LZSS_NBITS_RUN))
#define LZSS_MASK_RUN      ((1 << LZSS_NBITS_RUN) - 1)

usize lzss_decoded_size(const void *src)
{
    const lzss_header_s *head = (const lzss_header_s *)src;
    return head->size;
}

usize lzss_decode_ext(const void *src, void *dst, i32 bits_run)
{
    const lzss_header_s *head     = (const lzss_header_s *)src;
    const u8            *s        = (const u8 *)(head + 1);
    const u8            *s_end    = s + head->nbytes;
    byte                *d        = (byte *)dst;
    u32                  nblock   = 0; // block of flags to encode byte literals vs stream
    u32                  flags    = 0; // flag whether the next token is a single byte literal
    u32                  mask_run = ((u32)1 << bits_run) - 1;

    while (s < s_end) {
        if (nblock == 0) {
            flags = *s++;
        }

        if (flags & (1 << nblock)) { // flag set: byte literal
            *d++ = *s++;
        } else { // flat not set: copy run of bytes from an offset
            u32 v = 0;
            v |= *s++ << 8;
            v |= *s++;
            u32   off   = 1 + (v >> bits_run);
            u32   run   = LZSS_LIT_THRESHOLD + (v & mask_run);
            byte *d_cpy = d - off;

            for (u32 j = 0; j < run; j++) {
                *d++ = *d_cpy++;
            }
        }
        nblock = (nblock + 1) & 7; // increment bit index for flags
    }
    assert((usize)(d - (byte *)dst) == head->size); // does the decoded size match the size in the header?
    return head->size;
}

usize lzss_decode(const void *src, void *dst)
{
    return lzss_decode_ext(src, dst, LZSS_NBITS_RUN);
}

usize lzss_encode(const void *src, usize srcl, void *dst)
{
    lzss_header_s *head   = (lzss_header_s *)dst;
    const byte    *s      = (const byte *)src;
    u8            *d_beg  = (u8 *)(head + 1);
    u8            *d      = d_beg;
    u32            n      = 0; // index of source byte
    u32            nblock = 0; // block of flags to encode byte literals vs stream
    u8            *flags  = 0; // flag whether the next token is a single byte literal

    while (n < srcl) {
        u32 best_k = 0;
        u32 best_l = 0;

        // look back
        u32 k_search = 0 < n - LZSS_MAX_OFF ? n - LZSS_MAX_OFF : 0;
        for (u32 k = k_search; k < n; k++) {
            if (s[n] != s[k]) continue;

            u32 l = 1;
            while (l < LZSS_MAX_RUN && (usize)(n + l) < srcl && s[k + l] == s[n + l]) {
                l++;
            }

            if (best_l <= l) { // longest run so far, or less far back?
                best_k = k;
                best_l = l;
            }
        }

        if (nblock == 0) {
            flags  = d++;
            *flags = 0;
        }

        if (best_l < LZSS_LIT_THRESHOLD) { // literal
            *flags |= (1 << nblock);
            *d++ = s[n++];
        } else { // run
            u32 v = ((n - best_k - 1) << LZSS_NBITS_RUN) | (best_l - LZSS_LIT_THRESHOLD);
            *d++  = v >> 8;
            *d++  = v & 0xFF;
            n += best_l;
        }
        nblock = (nblock + 1) & 7;
    }

    usize size   = (usize)(d - (u8 *)dst);
    head->nbytes = (u32)(size - sizeof(lzss_header_s));
    head->size   = (u32)srcl;
    return size;
}

typedef struct {
    ALIGNAS(16)
    void *f;
    u32   i;       // current byte position in b
    u32   nbytes;  // bits left to decode
    u8    b[4096]; // working buffer
} lzss_file_decoder_s;

static u32 lzss_r_byte_stream(void *f, lzss_file_decoder_s *s)
{
    if (sizeof(s->b) <= s->i) {
        u32 to_read = s->nbytes < sizeof(s->b) ? s->nbytes : sizeof(s->b);
        pltf_file_r(f, s->b, (usize)to_read);
        s->i = 0;
    }
    s->nbytes--;
    return s->b[s->i++];
}

usize lzss_decode_file(void *f, void *dst)
{
    lzss_header_s head;
    if (!pltf_file_r_checked(f, &head, sizeof(lzss_header_s))) return 0;

    lzss_file_decoder_s s;
    s.f         = f;
    s.nbytes    = head.nbytes;
    s.i         = sizeof(s.b); // force buffer fill on first read
    u32   block = 0;
    u32   flags = 0;
    byte *d     = (byte *)dst;

    while (s.nbytes) {
        if (block == 0) {
            flags = lzss_r_byte_stream(f, &s);
        }

        u32 b = lzss_r_byte_stream(f, &s);
        if (flags & (1 << block)) {
            *d++ = b;
        } else {
            u32   v     = (b << 8) | lzss_r_byte_stream(f, &s);
            u32   off   = 1 + (v >> LZSS_NBITS_RUN);
            u32   run   = LZSS_LIT_THRESHOLD + (v & LZSS_MASK_RUN);
            byte *d_cpy = d - off;
            for (u32 j = 0; j < run; j++) {
                *d++ = *d_cpy++;
            }
        }
        block = (block + 1) & 7;
    }
    return (usize)(d - (byte *)dst);
}

usize lzss_decode_file_peek_size(void *f)
{
    lzss_header_s head;
    i32           p = pltf_file_tell(f);
    pltf_file_r(f, &head, sizeof(lzss_header_s));
    pltf_file_seek_set(f, p);
    return head.size;
}