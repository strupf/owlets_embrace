// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "util/bitrw.h"
#include "util/mathfunc.h"

// read up to 32 bits from the buffer
u32 bitrw_r_stream(bitrw_s *br, i32                                   nbits,
                   void (*on_new_word)(bitrw_s *br, void *ctx), void *ctx)
{
    // fast path and prevents undefined shifting behaviour below
    if (nbits == 32 && br->pos == 0) {
        u32 v = *br->b;
        br->b++;
        on_new_word(br, ctx);
        return v;
    }

    u32 x = 0;
    i32 n = nbits;
    while (n) {
        i32 s = min_i32(n, 32 - br->pos);                  // bits to read (in this word)
        u32 k = (*br->b >> br->pos) & (((u32)1 << s) - 1); // shift + mask read bits
        x     = (x << s) | k;                              // assemble bits to resulting value
        n -= s;
        br->pos += s;
        if (br->pos == 32) { // word is fully read, advance
            br->pos = 0;
            br->b++;
            on_new_word(br, ctx);
        }
    }
    return x;
}

i32 bitrw_r1_stream(bitrw_s *br,
                    void (*on_new_word)(bitrw_s *br, void *ctx), void *ctx)
{
    i32 x = (*br->b >> br->pos) & 1; // shift + mask read bits
    br->pos++;
    if (br->pos == 32) { // word is fully read, advance
        br->pos = 0;
        br->b++;
        on_new_word(br, ctx);
    }
    return x;
}

u32 bitrw_r(bitrw_s *br, i32 nbits)
{
    // fast path and prevents undefined shifting behaviour below
    if (nbits == 32 && br->pos == 0) {
        return (*br->b++);
    }

    u32 x = 0;
    i32 n = nbits;
    while (n) {
        i32 s = min_i32(n, 32 - br->pos);                  // bits to read (in this word)
        u32 k = (*br->b >> br->pos) & (((u32)1 << s) - 1); // shift + mask read bits
        x     = (x << s) | k;                              // assemble bits to resulting value
        n -= s;
        br->pos += s;
        if (br->pos == 32) { // word is fully read, advance
            br->pos = 0;
            br->b++;
        }
    }
    return x;
}

void bitrw_w(bitrw_s *bw, u32 val, i32 nbits)
{
    // fast path and prevents undefined shifting behaviour below
    if (nbits == 32 && bw->pos == 0) {
        *bw->b++ = val;
        return;
    }

    i32 n = nbits;
    u32 v = val;
    while (n) {
        if (bw->pos == 32) { // word is fully written, advance
            bw->pos = 0;
            bw->b++;
        }
        i32 s  = min_i32(n, 32 - bw->pos); // bits to read (in this word)
        u32 k  = v & (((u32)1 << s) - 1);  // mask bits to write
        *bw->b = (*bw->b << s) | k;        // assemble bits to output buffer
        bw->pos += s;                      // advance
        v >>= s;
        n -= s;
    }
}

i32 bitrw_r1(bitrw_s *br)
{
    i32 x = (*br->b >> br->pos) & 1; // shift + mask read bits
    br->pos++;
    if (br->pos == 32) { // word is fully read, advance
        br->pos = 0;
        br->b++;
    }
    return x;
}

void bitrw_w1(bitrw_s *bw, i32 val)
{
    if (bw->pos == 32) { // word is fully written, advance
        bw->pos = 0;
        bw->b++;
    }
    *bw->b = (*bw->b << 1) | (val & 1); // assemble bits to output buffer
    bw->pos++;                          // advance
}