// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "gfx.h"
#include "pltf/pltf.h"
#include "spm.h"
#include "util/json.h"
#include "util/mathfunc.h"
#include "util/sorting.h"
#include "util/str.h"

#define SPRBLIT_FUNCNAME gfx_spr_d_s
#define SPRBLIT_SRC_MASK 0
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 0
#include "gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_dm_sm
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 1
#define SPRBLIT_FLIPPEDX 0
#include "gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_sm
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 0
#include "gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_dm_sm_fx
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 1
#define SPRBLIT_FLIPPEDX 1
#include "gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_sm_fx
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 1
#include "gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME      gfx_spr_sm_copy
#define SPRBLIT_SRC_MASK      1
#define SPRBLIT_DST_MASK      0
#define SPRBLIT_FLIPPEDX      0
#define SPRBLIT_FUNCTION_COPY 1
#include "gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX
#undef SPRBLIT_FUNCTION_COPY

#define SPRBLIT_FUNCNAME      gfx_spr_sm_copy_fx
#define SPRBLIT_SRC_MASK      1
#define SPRBLIT_DST_MASK      0
#define SPRBLIT_FLIPPEDX      1
#define SPRBLIT_FUNCTION_COPY 1
#include "gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX
#undef SPRBLIT_FUNCTION_COPY

static const u32 g_bayer_8x8[65 * 8];

tex_s tex_framebuffer()
{
    tex_s t = {0};
    t.fmt   = TEX_FMT_OPAQUE;
    t.px    = (u32 *)pltf_1bit_buffer();
    t.w     = PLTF_DISPLAY_W;
    t.h     = PLTF_DISPLAY_H;
    t.wword = PLTF_DISPLAY_WWORDS;
    return t;
}

tex_s tex_create(i32 w, i32 h, b32 mask, allocator_s a, err32 *err)
{
    tex_s t        = {0};
    b32   m        = mask != 0;
    u32   waligned = (w + 31) & ~31;
    u32   wword    = (waligned >> 5) << (i32)m;
    u32   size     = sizeof(u32) * wword * h;
    void *mem      = a.allocfunc(a.ctx, size, 32);
    if (mem) {
        t.px    = (u32 *)mem;
        t.fmt   = m;
        t.w     = w;
        t.h     = h;
        t.wword = wword;
        if (err) {
            *err = 0;
        }
    } else if (err) {
        *err = 1;
    }
    return t;
}

texrec_s texrec_from_tex(tex_s t)
{
    texrec_s tr = {t, 0, 0, t.w, t.h};
    return tr;
}

static i32 tex_px_at_unsafe(tex_s tex, i32 x, i32 y)
{
    u32 b = bswap32(0x80000000 >> (x & 31));
    switch (tex.fmt) {
    case TEX_FMT_MASK: return (tex.px[y * tex.wword + ((x >> 5) << 1)] & b);
    case TEX_FMT_OPAQUE: return (tex.px[y * tex.wword + (x >> 5)] & b);
    }
    return 0;
}

i32 tex_mk_at_unsafe(tex_s tex, i32 x, i32 y)
{
    if (tex.fmt == TEX_FMT_OPAQUE) return 1;

    u32 b = bswap32(0x80000000 >> (x & 31));
    return (tex.px[y * tex.wword + ((x >> 5) << 1) + 1] & b);
}

void tex_px_unsafe(tex_s tex, i32 x, i32 y, i32 col)
{
    u32  b = bswap32(0x80000000 >> (x & 31));
    u32 *p = 0;
    switch (tex.fmt) {
    case TEX_FMT_MASK: p = &tex.px[y * tex.wword + ((x >> 5) << 1)]; break;
    case TEX_FMT_OPAQUE: p = &tex.px[y * tex.wword + (x >> 5)]; break;
    default: return;
    }
    *p = (col == 0 ? *p & ~b : *p | b);
}

void tex_px_unsafe_display(tex_s tex, i32 x, i32 y, i32 col)
{
    u32  b = bswap32(0x80000000 >> (x & 31));
    u32 *p = &tex.px[y * tex.wword + (x >> 5)];
    *p     = (col == 0 ? *p & ~b : *p | b);
}

static void tex_mk_unsafe(tex_s tex, i32 x, i32 y, i32 col)
{
    if (tex.fmt == TEX_FMT_OPAQUE) return;
    u32  b = bswap32(0x80000000 >> (x & 31));
    u32 *p = &tex.px[y * tex.wword + ((x >> 5) << 1) + 1];
    *p     = (col == 0 ? *p & ~b : *p | b);
}

i32 tex_px_at(tex_s tex, i32 x, i32 y)
{
    if (!(0 <= x && x < tex.w && 0 <= y && y < tex.h)) return 0;
    return tex_px_at_unsafe(tex, x, y);
}

i32 tex_mk_at(tex_s tex, i32 x, i32 y)
{
    if (!(0 <= x && x < tex.w && 0 <= y && y < tex.h)) return 1;
    return tex_mk_at_unsafe(tex, x, y);
}

void tex_px(tex_s tex, i32 x, i32 y, i32 col)
{
    if (0 <= x && x < tex.w && 0 <= y && y < tex.h) {
        tex_px_unsafe(tex, x, y, col);
    }
}

void tex_mk(tex_s tex, i32 x, i32 y, i32 col)
{
    if (0 <= x && x < tex.w && 0 <= y && y < tex.h) {
        tex_mk_unsafe(tex, x, y, col);
    }
}

// only works if x and w are multiple of 32 right now
void tex_outline_col_small(tex_s tex, i32 col)
{
    // use stack mem instead (faster)
    assert(tex.fmt == TEX_FMT_MASK);
    i32   wword = tex.wword >> 1;
    usize size  = sizeof(u32) * wword * tex.h;
    assert(size <= 4096);
    u32 sm[1024];

    for (i32 n = 0, k = 1; n < wword * tex.h; n++, k += 2) {
        sm[n] = bswap32(tex.px[k]);
    }

    i32 x1 = 0;
    i32 y1 = 0;
    i32 x2 = wword - 1;
    i32 y2 = tex.h - 1;

    for (i32 yy = y1; yy <= y2; yy++) {
        i32 yi1 = yy == y1 ? 0 : -1;
        i32 yi2 = yy == y2 ? 0 : +1;

        for (i32 xx = x1; xx <= x2; xx++) {
            u32 m = 0;

            for (i32 yi = yi1; yi <= yi2; yi++) {
                i32 v = (yy + yi) * wword;
                u32 k = sm[xx + v];
                m |= k | (k >> 1) | (k << 1);

                if (x1 < xx) {
                    u32 kl = sm[xx + v - 1];
                    m |= kl << 31;
                }
                if (xx < x2) {
                    u32 kr = sm[xx + v + 1];
                    m |= kr >> 31;
                }
            }
            if (m) {
                u32 *dp = &tex.px[(xx << 1) + (yy * tex.wword)];
                u32 *dm = dp + 1;
                m       = bswap32(m) & ~*dm;
                *dm |= m;
                switch (col) {
                case GFX_COL_WHITE: *dp |= m; break;
                case GFX_COL_BLACK: *dp &= ~m; break;
                }
            }
        }
    }
}

void tex_outline_col_ext_small(tex_s tex, i32 col, b32 dia)
{
    assert(tex.fmt == TEX_FMT_MASK);
    i32   wword = tex.wword >> 1;
    usize size  = sizeof(u32) * wword * tex.h;
    assert(size <= 4096);
    u32 sm[1024];
    for (i32 n = 0, k = 1; n < wword * tex.h; n++, k += 2) {
        sm[n] = bswap32(tex.px[k]);
    }

    i32 x1 = 0;
    i32 y1 = 0;
    i32 x2 = wword - 1;
    i32 y2 = tex.h - 1;

    for (i32 yy = y1; yy <= y2; yy++) {
        i32 yi1 = yy == y1 ? 0 : -1;
        i32 yi2 = yy == y2 ? 0 : +1;

        for (i32 xx = x1; xx <= x2; xx++) {
            u32 m = 0;

            for (i32 yi = yi1; yi <= yi2; yi++) {
                i32 v = (yy + yi) * wword;
                u32 k = sm[xx + v];
                m |= k;

                // neighbour pixels
                if (dia || yi == 0) {
                    m |= k >> 1;
                    m |= k << 1;

                    if (x1 < xx) {
                        m |= sm[xx + v - 1] << 31;
                    }
                    if (xx < x2) {
                        m |= sm[xx + v + 1] >> 31;
                    }
                }
            }
            if (m) {
                u32 *dp = &tex.px[(xx << 1) + (yy * tex.wword)];
                u32 *dm = dp + 1;
                m       = bswap32(m) & ~*dm;
                *dm |= m;
                *dp = col ? (*dp | m) : (*dp & ~m);
            }
        }
    }
}

// only works if x and w are multiple of 32 right now
void tex_outline_col_ext(tex_s tex, i32 col, b32 dia)
{
    assert(tex.fmt == TEX_FMT_MASK);
    spm_push();
    i32   wword = tex.wword >> 1;
    usize size  = sizeof(u32) * wword * tex.h;
    u32  *sm    = (u32 *)spm_alloc_aligned(size, 4);
    for (i32 n = 0, k = 1; n < wword * tex.h; n++, k += 2) {
        sm[n] = bswap32(tex.px[k]);
    }

    i32 x1 = 0;
    i32 y1 = 0;
    i32 x2 = wword - 1;
    i32 y2 = tex.h - 1;

    for (i32 yy = y1; yy <= y2; yy++) {
        i32 yi1 = yy == y1 ? 0 : -1;
        i32 yi2 = yy == y2 ? 0 : +1;

        for (i32 xx = x1; xx <= x2; xx++) {
            u32 m = 0;

            for (i32 yi = yi1; yi <= yi2; yi++) {
                i32 v = (yy + yi) * wword;
                u32 k = sm[xx + v];
                m |= k;

                // neighbour pixels
                if (dia || yi == 0) {
                    m |= k >> 1;
                    m |= k << 1;

                    if (x1 < xx) {
                        m |= sm[xx + v - 1] << 31;
                    }
                    if (xx < x2) {
                        m |= sm[xx + v + 1] >> 31;
                    }
                }
            }
            if (m) {
                u32 *dp = &tex.px[(xx << 1) + (yy * tex.wword)];
                u32 *dm = dp + 1;
                m       = bswap32(m) & ~*dm;
                *dm |= m;
                *dp = col ? (*dp | m) : (*dp & ~m);
            }
        }
    }
    spm_pop();
}

void tex_outline_col(tex_s tex, i32 col)
{
    tex_outline_col_ext(tex, col, 1);
}

// only works if x and w are multiple of 32 right now
void tex_outline_white_small(tex_s tex)
{
    tex_outline_col_small(tex, GFX_COL_WHITE);
}

// only works if x and w are multiple of 32 right now
void tex_outline_black_small(tex_s tex)
{
    tex_outline_col_small(tex, GFX_COL_BLACK);
}

// only works if x and w are multiple of 32 right now
void tex_outline_white(tex_s tex)
{
    tex_outline_col(tex, GFX_COL_WHITE);
}

// only works if x and w are multiple of 32 right now
void tex_outline_black(tex_s tex)
{
    tex_outline_col(tex, GFX_COL_BLACK);
}

void tex_merge_to_opaque(tex_s dst, tex_s src)
{
    assert(dst.fmt == TEX_FMT_OPAQUE);
    assert(src.fmt == TEX_FMT_MASK);
    assert(dst.wword * 2 == src.wword && dst.h == src.h);

    u32 *d = dst.px;
    u32 *s = src.px;
    for (i32 n = 0; n < dst.wword * dst.h; n++) {
        *d = (*d & ~s[1]) | (s[0] & s[1]); // copy mode
        d += 1;
        s += 2;
    }
}

void tex_merge_to_opaque_outlined_white(tex_s dst, tex_s src)
{
    assert(dst.fmt == TEX_FMT_OPAQUE);
    assert(src.fmt == TEX_FMT_MASK);
    assert(dst.wword * 2 == src.wword && dst.h == src.h);

    u32 *d = dst.px;
    u32 *s = src.px;

    for (i32 y = 0; y < src.h; y++) {
        i32 yi1 = y == 0 ? 0 : -1;
        i32 yi2 = y == src.h - 1 ? 0 : +1;

        for (i32 x = 0; x < src.wword; x += 2) {
            u32 sp = *(s + 0);
            u32 sm = *(s + 1);

            if (sm != 0xFFFFFFFFU) {
                u32 m = 0;

                for (i32 yi = yi1; yi <= yi2; yi++) {
                    i32 v = 1 + yi * src.wword;
                    {
                        u32 k = bswap32(*(s + v));
                        m |= k | (k >> 1) | (k << 1);
                    }
                    if (0 <= (x - 2)) {
                        m |= bswap32(*(s + v - 2)) << 31;
                    }
                    if ((x + 2) < src.wword) {
                        m |= bswap32(*(s + v + 2)) >> 31;
                    }
                }
                m = bswap32(m) & ~sm; // white outline only on transparency
                sp |= m;
                sm |= m;
            }
            *d = (*d & ~sm) | (sp & sm); // copy mode
            d += 1;
            s += 2;
        }
    }
}

gfx_ctx_s gfx_ctx_default(tex_s dst)
{
    gfx_ctx_s ctx = {0};
    ctx.dst       = dst;
    ctx.clip_x2   = dst.w - 1;
    ctx.clip_y2   = dst.h - 1;
    mset(&ctx.pat, 0xFF, sizeof(gfx_pattern_s));
    return ctx;
}

gfx_ctx_s gfx_ctx_display()
{
    return gfx_ctx_default(tex_framebuffer());
}

gfx_ctx_s gfx_ctx_unclip(gfx_ctx_s ctx)
{
    gfx_ctx_s c = ctx;
    c.clip_x1   = 0;
    c.clip_y1   = 0;
    ctx.clip_x2 = ctx.dst.w - 1;
    ctx.clip_y2 = ctx.dst.h - 1;
    return c;
}

gfx_ctx_s gfx_ctx_clip(gfx_ctx_s ctx, i32 x1, i32 y1, i32 x2, i32 y2)
{
    gfx_ctx_s c = ctx;
    c.clip_x1   = max_i32(x1, 0);
    c.clip_y1   = max_i32(y1, 0);
    c.clip_x2   = min_i32(x2, ctx.dst.w - 1);
    c.clip_y2   = min_i32(y2, ctx.dst.h - 1);
    return c;
}

gfx_ctx_s gfx_ctx_clip_top(gfx_ctx_s ctx, i32 y1)
{
    gfx_ctx_s c = ctx;
    c.clip_y1   = max_i32(y1, 0);
    return c;
}

gfx_ctx_s gfx_ctx_clip_bot(gfx_ctx_s ctx, i32 y2)
{
    gfx_ctx_s c = ctx;
    c.clip_y2   = min_i32(y2, ctx.dst.h - 1);
    return c;
}

gfx_ctx_s gfx_ctx_clip_left(gfx_ctx_s ctx, i32 x1)
{
    gfx_ctx_s c = ctx;
    c.clip_x1   = max_i32(x1, 0);
    return c;
}

gfx_ctx_s gfx_ctx_clip_right(gfx_ctx_s ctx, i32 x2)
{
    gfx_ctx_s c = ctx;
    c.clip_x2   = min_i32(x2, ctx.dst.w - 1);
    return c;
}

gfx_ctx_s gfx_ctx_clipr(gfx_ctx_s ctx, rec_i32 r)
{
    return gfx_ctx_clip(ctx, r.x, r.y, r.x + r.w - 1, r.y + r.h - 1);
}

gfx_ctx_s gfx_ctx_clipwh(gfx_ctx_s ctx, i32 x, i32 y, i32 w, i32 h)
{
    return gfx_ctx_clip(ctx, x, y, x + w - 1, x + h - 1);
}

gfx_pattern_s gfx_pattern_inv(gfx_pattern_s p)
{
    gfx_pattern_s r = {0};
    for (i32 i = 0; i < 8; i++) {
        r.p[i] = ~p.p[i];
    }
    return r;
}

// 01
gfx_pattern_s gfx_pattern_2x2(i32 p0, i32 p1)
{
    gfx_pattern_s pat  = {0};
    u32           p[2] = {p0, p1};
    for (i32 i = 0; i < 2; i++) {
        u32 pa       = (p[i] << 6) | (p[i] << 4) | (p[i] << 2) | (p[i]);
        u32 pb       = (pa << 24) | (pa << 16) | (pa << 8) | (pa);
        pat.p[i + 0] = pb;
        pat.p[i + 2] = pb;
        pat.p[i + 4] = pb;
        pat.p[i + 6] = pb;
    }
    return pat;
}

gfx_pattern_s gfx_pattern_4x4(i32 p0, i32 p1, i32 p2, i32 p3)
{
    gfx_pattern_s pat  = {0};
    u32           p[4] = {p0, p1, p2, p3};
    for (i32 i = 0; i < 4; i++) {
        u32 pa       = (p[i] << 4) | (p[i]);
        u32 pb       = (pa << 24) | (pa << 16) | (pa << 8) | (pa);
        pat.p[i + 0] = pb;
        pat.p[i + 4] = pb;
    }
    return pat;
}

gfx_pattern_s gfx_pattern_bayer_4x4(i32 i)
{
    // the 4x4 patterns are mapped to a table of 8x8 patterns
    // index of equivalent 8x8 = 4 * (index of 4x4)
    gfx_pattern_s pat = {0};
    i32           n   = clamp_i32(i, 0, 16); // pattern index
    mcpy(pat.p, &g_bayer_8x8[n << 5], sizeof(gfx_pattern_s));
    return pat;
}

gfx_pattern_s gfx_pattern_bayer_8x8(i32 i)
{
    gfx_pattern_s pat = {0};
    i32           n   = clamp_i32(i, 0, 64); // pattern index
    mcpy(pat.p, &g_bayer_8x8[n << 3], sizeof(gfx_pattern_s));
    return pat;
}

gfx_pattern_s gfx_pattern_shift(gfx_pattern_s p, i32 x, i32 y)
{
    gfx_pattern_s pat = {0};
    i32           s   = (x & 7);

    for (i32 i = 0; i < 8; i++) {
        u32 k    = p.p[(i - y) & 7];
        pat.p[i] = (k >> s) | (k << (8 - s));
    }
    return pat;
}

gfx_pattern_s gfx_pattern_interpolate(i32 num, i32 den)
{
    return gfx_pattern_bayer_4x4((num * GFX_PATTERN_NUM) / den);
}

gfx_pattern_s gfx_pattern_interpolatec(i32 num, i32 den, i32 (*ease)(i32 a, i32 b, i32 num, i32 den))
{
    i32 i = ease(0, 16, num, den);
    return gfx_pattern_bayer_4x4(i);
}

void tex_clr(tex_s dst, i32 col)
{
    i32  N = dst.wword * dst.h;
    u32 *p = dst.px;
    if (!p) return;

    switch (col) {
    case GFX_COL_BLACK:
        switch (dst.fmt) {
        case TEX_FMT_OPAQUE:
            mclr(p, sizeof(u32) * N);
            break;
        case TEX_FMT_MASK:
            for (i32 n = 0; n < N; n += 2) {
                *p++ = 0;          // data
                *p++ = 0xFFFFFFFF; // mask
            }
            break;
        }
        break;
    case GFX_COL_WHITE:
        switch (dst.fmt) {
        case TEX_FMT_OPAQUE:
            mset(p, 0xFF, sizeof(u32) * N);
            break;
        case TEX_FMT_MASK:
            mset(p, 0xFF, sizeof(u32) * N);
            break;
        }
        break;
    case GFX_COL_CLEAR:
        if (dst.fmt == TEX_FMT_OPAQUE) break;
        mclr(p, sizeof(u32) * N);
        break;
    }
}

typedef struct {
    gfx_pattern_s pat;
    u32          *dp; // pixel
    u32           ml; // boundary mask left
    u32           mr; // boundary mask right
    u16           dst_wword;
    i16           y;
    u8            dmax; // count of dst words -1
    u8            mode; // drawing mode
    u8            dadd;
    u8            doff; // bitoffset of first dst bit
} span_blit_s;

static span_blit_s span_blit_change_y(span_blit_s s, i32 y)
{
    span_blit_s r = s;
    r.dp          = s.dp + (y - s.y) * s.dst_wword;
    r.y           = y;
    return r;
}

static span_blit_s span_blit_gen(gfx_ctx_s ctx, i32 y, i32 x1, i32 x2, i32 mode)
{
    i32         nbit = (x2 + 1) - x1; // number of bits in a row to blit
    i32         lsh  = (ctx.dst.fmt == TEX_FMT_MASK);
    span_blit_s info = {0};
    info.y           = y;
    info.doff        = x1 & 31;
    info.dmax        = (info.doff + nbit - 1) >> 5;                        // number of touched dst words -1
    info.mode        = mode;                                               // sprite masking mode
    info.ml          = bswap32(0xFFFFFFFFU >> (31 & info.doff));           // mask to cut off boundary left
    info.mr          = bswap32(0xFFFFFFFFU << (31 & (-info.doff - nbit))); // mask to cut off boundary right
    info.dst_wword   = ctx.dst.wword;
    info.dp          = &ctx.dst.px[((x1 >> 5) << lsh) + y * ctx.dst.wword];
    info.dadd        = 1 + lsh;
    info.pat         = ctx.pat;
    return info;
}

static inline void span_blit_incr_y(span_blit_s *info)
{
    info->y++;
    info->dp += info->dst_wword;
}

void gfx_spr_tileds(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode, bool32 tilex, bool32 tiley)
{
    if (!src.t.px || (src.w | src.h) == 0) return;

    i32 x1 = tilex ? ((pos.x % src.w) - src.w) % src.w : pos.x;
    i32 y1 = tiley ? ((pos.y % src.h) - src.h) % src.h : pos.y;
    i32 x2 = tilex ? ((ctx.dst.w - x1) / src.w) * src.w : x1;
    i32 y2 = tiley ? ((ctx.dst.h - y1) / src.h) * src.h : y1;

    for (i32 y = y1; y <= y2; y += src.h) {
        for (i32 x = x1; x <= x2; x += src.w) {
            v2_i32 p = {x, y};
            gfx_spr(ctx, src, p, flip, mode);
        }
    }
}

void gfx_spr_tileds_copy(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, bool32 tilex, bool32 tiley)
{
    if (!src.t.px || (src.w | src.h) == 0) return;

    i32 x1 = tilex ? ((pos.x % src.w) - src.w) % src.w : pos.x;
    i32 y1 = tiley ? ((pos.y % src.h) - src.h) % src.h : pos.y;
    i32 x2 = tilex ? ((ctx.dst.w - x1) / src.w) * src.w : x1;
    i32 y2 = tiley ? ((ctx.dst.h - y1) / src.h) * src.h : y1;

    for (i32 y = y1; y <= y2; y += src.h) {
        for (i32 x = x1; x <= x2; x += src.w) {
            v2_i32 p = {x, y};
            gfx_spr_sm_copy(ctx, src, p, 0, 0);
        }
    }
}

static void apply_prim_mode(u32 *dp, u32 *dm, u32 sm, i32 mode, u32 pt)
{
    switch (mode) {
    case PRIM_MODE_INV: sm &= pt, *dp = (*dp & ~sm) | (~*dp & sm); break;
    case PRIM_MODE_WHITE: sm &= pt, *dp |= sm; break;
    case PRIM_MODE_BLACK: sm &= pt, *dp &= ~sm; break;
    case PRIM_MODE_WHITE_BLACK: pt = ~pt; // fallthrough
    case PRIM_MODE_BLACK_WHITE: *dp = (*dp & ~(sm & pt)) | (sm & ~pt); break;
    case PRIM_MODE_WHITEN: sm &= pt, *dp |= (sm); break;
    case PRIM_MODE_BLACKEN: sm &= pt, *dp &= ~(sm); break;
    }

    if (mode != PRIM_MODE_WHITEN &&
        mode != PRIM_MODE_BLACKEN &&
        mode != PRIM_MODE_INV &&
        dm) {
        *dm |= sm;
    }
}

static void prim_blit_span(span_blit_s info)
{
    u32 *dp = (u32 *)info.dp;
    u32  pt = info.pat.p[info.y & 7];
    u32  m  = info.ml;
    for (i32 i = 0; i < info.dmax; i++) {
        apply_prim_mode(dp, info.dadd == 2 ? dp + 1 : 0, m, info.mode, pt);
        m = 0xFFFFFFFFU;
        dp += info.dadd;
    }
    apply_prim_mode(dp, info.dadd == 2 ? dp + 1 : 0, m & info.mr, info.mode, pt);
}

static void apply_prim_mode_X(u32 *dp, u32 sm, i32 mode, u32 pt)
{
    switch (mode) {
    case PRIM_MODE_INV: sm &= pt, *dp = (*dp & ~sm) | (~*dp & sm); break;
    case PRIM_MODE_WHITE: sm &= pt, *dp |= sm; break;
    case PRIM_MODE_BLACK: sm &= pt, *dp &= ~sm; break;
    case PRIM_MODE_WHITE_BLACK: pt = ~pt; // fallthrough
    case PRIM_MODE_BLACK_WHITE: *dp = (*dp & ~(sm & pt)) | (sm & ~pt); break;
    case PRIM_MODE_WHITEN: sm &= pt, *dp |= (sm); break;
    case PRIM_MODE_BLACKEN: sm &= pt, *dp &= ~(sm); break;
    }
}

static void prim_blit_span_X(span_blit_s info)
{
    u32 *dp = (u32 *)info.dp;
    u32  pt = info.pat.p[info.y & 7];
    u32  m  = info.ml;

    for (i32 i = 0; i < info.dmax; i++) {
        apply_prim_mode_X(dp, m, info.mode, pt);
        m = 0xFFFFFFFFU;
        dp++;
    }
    apply_prim_mode_X(dp, m & info.mr, info.mode, pt);
}

static void prim_blit_span_Y(span_blit_s info)
{
    u32 *dp = (u32 *)info.dp;
    u32  pt = info.pat.p[info.y & 7];
    u32  m  = info.ml;

    for (i32 i = 0; i < info.dmax; i++) {
        apply_prim_mode(dp, dp + 1, m, info.mode, pt);
        m = 0xFFFFFFFFU;
        dp += 2;
    }
    apply_prim_mode(dp, dp + 1, m & info.mr, info.mode, pt);
}

void gfx_rec_fill(gfx_ctx_s ctx, rec_i32 rec, i32 mode)
{
    i32 x1 = max_i32(rec.x, ctx.clip_x1); // area bounds on canvas [x1/y1, x2/y2]
    i32 y1 = max_i32(rec.y, ctx.clip_y1);
    i32 x2 = min_i32(rec.x + rec.w - 1, ctx.clip_x2);
    i32 y2 = min_i32(rec.y + rec.h - 1, ctx.clip_y2);
    if (x2 < x1) return;

    tex_s       dtex = ctx.dst;
    span_blit_s info = span_blit_gen(ctx, y1, x1, x2, mode);
    if (dtex.fmt == TEX_FMT_OPAQUE) {
        for (i32 y = y1; y <= y2; y++) {
            prim_blit_span_X(info);
            span_blit_incr_y(&info);
        }
    } else {
        for (i32 y = y1; y <= y2; y++) {
            prim_blit_span_Y(info);
            span_blit_incr_y(&info);
        }
    }
}

void gfx_rec_fill_opaque(gfx_ctx_s ctx, rec_i32 rec, i32 mode)
{
    tex_s dtex = ctx.dst;
    assert(dtex.fmt == TEX_FMT_OPAQUE);

    i32 x1 = max_i32(rec.x, ctx.clip_x1); // area bounds on canvas [x1/y1, x2/y2]
    i32 y1 = max_i32(rec.y, ctx.clip_y1);
    i32 x2 = min_i32(rec.x + rec.w - 1, ctx.clip_x2);
    i32 y2 = min_i32(rec.y + rec.h - 1, ctx.clip_y2);
    if (x2 < x1) return;

    span_blit_s info = span_blit_gen(ctx, y1, x1, x2, mode);
    for (i32 y = y1; y <= y2; y++) {
        prim_blit_span_X(info);
        span_blit_incr_y(&info);
    }
}

void gfx_rec_strip(gfx_ctx_s ctx, i32 rx, i32 ry, i32 rw, i32 mode)
{
    i32 x1 = max_i32(rx, ctx.clip_x1); // area bounds on canvas [x1/y1, x2/y2]
    i32 x2 = min_i32(rx + rw - 1, ctx.clip_x2);
    if (x2 < x1 || ry < ctx.clip_y1 || ry > ctx.clip_y2) return;

    tex_s       dtex = ctx.dst;
    span_blit_s info = span_blit_gen(ctx, ry, x1, x2, mode);
    if (dtex.fmt == TEX_FMT_OPAQUE) {
        prim_blit_span_X(info);
    } else {
        prim_blit_span_Y(info);
    }
}

void gfx_rec_rounded_fill(gfx_ctx_s ctx, rec_i32 rec, i32 r, i32 mode)
{
    i32 rr = r < 0 ? rec.h / 2 : min_i32(r, rec.h / 2);
    i32 y1 = max_i32(ctx.clip_y1 - rec.y, 0);
    i32 y2 = min_i32(ctx.clip_y2 - rec.y, rec.h - 1);

    for (i32 y = y1; y < rr; y++) {
        i32     dx    = rr - sqrt_u32(pow2_i32(rr) - pow2_i32(rr - y) + 1);
        rec_i32 rline = {rec.x + dx, y + rec.y, rec.w - (dx << 1), 1};
        gfx_rec_fill(ctx, rline, mode);
    }

    rec_i32 rnormal = {rec.x, rec.y + rr, rec.w, rec.h - (rr << 1)};
    gfx_rec_fill(ctx, rnormal, mode);

    i32 t = rec.h - rr - 1;
    for (i32 y = t + 1; y <= y2; y++) {
        i32     dx    = rr - sqrt_u32(pow2_i32(rr) - pow2_i32(y - t) + 1);
        rec_i32 rline = {rec.x + dx, y + rec.y, rec.w - (dx << 1), 1};
        gfx_rec_fill(ctx, rline, mode);
    }
}

void gfx_fill_rows(tex_s dst, gfx_pattern_s pat, i32 y1, i32 y2)
{
    assert(0 <= y1 && y2 < dst.h);
    u32 *px = &dst.px[y1 * dst.wword];
    for (i32 y = y1; y <= y2; y++) {
        u32 p = ~pat.p[y & 7];
        for (i32 x = 0; x < dst.wword; x++) {
            *px++ = p;
        }
    }
}

void fnt_draw_str(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, const void *s, i32 mode)
{
    v2_i32   p = pos;
    texrec_s t = {0};
    t.t        = fnt.t;
    t.w        = fnt.grid_w;
    t.h        = fnt.grid_h;

    for (const u8 *c = (const u8 *)s; *c; c++) {
        i32 i = *(c);
        if (i && i < 127) {
            t.x = (i & 31) * fnt.grid_w;
            t.y = (i >> 5) * fnt.grid_h;
            gfx_spr(ctx, t, p, 0, mode);
        }

        p.x += fnt.widths[i] + fnt.tracking - fnt_kerning(fnt, i, *(c + 1));
    }
}

void fnt_draw_outline_style(gfx_ctx_s ctx, fnt_s f, v2_i32 pos, const void *str, i32 style, b32 centeredx)
{
#define FNT_STYLE_TEXW 320
#define FNT_STYLE_TEXH 40
    TEX_STACK_CTX(textmp, FNT_STYLE_TEXW, FNT_STYLE_TEXH, 1);

    switch (style) {
    case 0: {
        fnt_draw_str(textmp_ctx, f, (v2_i32){4, 4}, str, SPR_MODE_INV);
        tex_outline_col_ext(textmp, GFX_COL_BLACK, 1);
        tex_outline_col_ext(textmp, GFX_COL_BLACK, 0);
        tex_outline_col_ext(textmp, GFX_COL_WHITE, 1);
        break;
    }
    case 1: {
        fnt_draw_str(textmp_ctx, f, (v2_i32){4, 4}, str, 0);
        tex_outline_col_ext(textmp, GFX_COL_WHITE, 1);
        tex_outline_col_ext(textmp, GFX_COL_WHITE, 0);
        tex_outline_col_ext(textmp, GFX_COL_BLACK, 1);
        break;
    }
    case 2: {
        fnt_draw_str(textmp_ctx, f, (v2_i32){4, 4}, str, 0);
        tex_outline_col_ext(textmp, GFX_COL_WHITE, 1);
        tex_outline_col_ext(textmp, GFX_COL_WHITE, 0);
        // tex_outline_col_ext_small(textmp, GFX_COL_BLACK, 1);
        break;
    }
    case 3: {
        fnt_draw_str(textmp_ctx, f, (v2_i32){4, 4}, str, 0);
        tex_outline_col_ext(textmp, GFX_COL_WHITE, 1);
        break;
    }
    case 4: {
        fnt_draw_str(textmp_ctx, f, (v2_i32){4, 4}, str, SPR_MODE_INV);
        tex_outline_col_ext(textmp, GFX_COL_BLACK, 1);
        tex_outline_col_ext(textmp, GFX_COL_BLACK, 0);
        break;
    }
    case 5: {
        fnt_draw_str(textmp_ctx, f, (v2_i32){4, 4}, str, 0);
        tex_outline_col_ext(textmp, GFX_COL_WHITE, 1);
        tex_outline_col_ext(textmp, GFX_COL_BLACK, 1);
        tex_outline_col_ext(textmp, GFX_COL_BLACK, 0);
        break;
    }
    case 6: {
        fnt_draw_str(textmp_ctx, f, (v2_i32){4, 4}, str, SPR_MODE_INV);
        break;
    }
    case 7: {
        fnt_draw_str(textmp_ctx, f, (v2_i32){4, 4}, str, 0);
        break;
    }
    case 8: {
        fnt_draw_str(textmp_ctx, f, (v2_i32){4, 4}, str, 0);
        tex_outline_col_ext(textmp, GFX_COL_WHITE, 1);
        tex_outline_col_ext(textmp, GFX_COL_BLACK, 1);
        break;
    }
    }

    i32 px = centeredx ? fnt_length_px(f, str) : 0;
    gfx_spr(ctx, texrec_from_tex(textmp), (v2_i32){pos.x - 4 - (px >> 1), pos.y - 2}, 0, 0);
}

i32 fnt_length_px(fnt_s fnt, const void *txt)
{
    i32 l = 0;
    for (const u8 *c = (const u8 *)txt; *c; c++) {
        i32 ci = *(c);
        l += fnt.widths[ci] + fnt.tracking - fnt_kerning(fnt, ci, *(c + 1));
    }
    return l;
}

i32 fnt_kerning(fnt_s fnt, i32 c1, i32 c2)
{
    if (c1 != '\0' && c2 != '\0') {
        for (i32 k = 0; k < fnt.n_kerning; k++) {
            fnt_kerning_s *fk = &fnt.kerning[k];
            if (c1 == fk->c1 && c2 == fk->c2) {
                return (i32)fk->space;
            }
        }
    }
    return 0;
}

void gfx_tri_fill(gfx_ctx_s ctx, tri_i32 t, i32 mode)
{
    v2_i32 t0 = t.p[0];
    v2_i32 t1 = t.p[1];
    v2_i32 t2 = t.p[2];
    if (t0.y > t1.y) SWAP(v2_i32, t0, t1);
    if (t0.y > t2.y) SWAP(v2_i32, t0, t2);
    if (t1.y > t2.y) SWAP(v2_i32, t1, t2);
    i32 th = t2.y - t0.y;
    if (th == 0) return;
    i32 h1  = t1.y - t0.y + 1;
    i32 h2  = t2.y - t1.y + 1;
    i32 d0  = t2.x - t0.x;
    i32 d1  = t1.x - t0.x;
    i32 d2  = t2.x - t1.x;
    i32 ya0 = max_i32(ctx.clip_y1, t0.y);
    i32 ya1 = min_i32(ctx.clip_y2, t1.y);

    for (i32 y = ya0; y <= ya1; y++) {
        assert(ctx.clip_y1 <= y && y <= ctx.clip_y2);
        i32 x1 = t0.x + (d0 * (y - t0.y)) / th;
        i32 x2 = t0.x + (d1 * (y - t0.y)) / h1;
        if (x2 < x1) SWAP(i32, x1, x2);
        x1 = max_i32(x1, ctx.clip_x1);
        x2 = min_i32(x2, ctx.clip_x2);
        if (x2 < x1) continue;
        span_blit_s info = span_blit_gen(ctx, y, x1, x2, mode);
        prim_blit_span(info);
    }

    i32 yb0 = max_i32(ctx.clip_y1, t1.y);
    i32 yb1 = min_i32(ctx.clip_y2, t2.y);

    for (i32 y = yb0; y <= yb1; y++) {
        assert(ctx.clip_y1 <= y && y <= ctx.clip_y2);
        i32 x1 = t0.x + (d0 * (y - t0.y)) / th;
        i32 x2 = t1.x + (d2 * (y - t1.y)) / h2;
        if (x2 < x1) SWAP(i32, x1, x2);
        x1 = max_i32(x1, ctx.clip_x1);
        x2 = min_i32(x2, ctx.clip_x2);
        if (x2 < x1) continue;
        span_blit_s info = span_blit_gen(ctx, y, x1, x2, mode);
        prim_blit_span(info);
    }
}

// triangle pixels in Q2 - subpixel accuracy!
// naive per pixel loop from my own software renderer
// whole spans for playdate
void gfx_tri_fill_uvw(gfx_ctx_s ctx, v2_i32 tri[3], i32 mode)
{
    v2_i32 vv[3] = {0};
    for (i32 n = 0; n < 3; n++) {
        vv[n].x = tri[n].x;
        vv[n].y = tri[n].y;
    }

    v2_i32 v12 = v2_i32_sub(vv[2], vv[1]);
    v2_i32 v20 = v2_i32_sub(vv[0], vv[2]);
    v2_i32 v01 = v2_i32_sub(vv[1], vv[0]);
    i32    den = v2_i32_crs(v01, v20); // den is max: (w * subpx) * (h * subpx)

    if (den == 0) return; // invalid triangle
    if (den < 0) {
        den = -den;
        v12 = v2_i32_inv(v12);
        v20 = v2_i32_inv(v20);
        v01 = v2_i32_inv(v01);
    }

    i32 xa = max_i32(min3_i32(v20.x, v12.x, v01.x) >> 2, 0);           // screen x1
    i32 ya = max_i32(min3_i32(v20.y, v12.y, v01.y) >> 2, 0);           // screen y1
    i32 xb = min_i32(max3_i32(v20.x, v12.x, v01.x) >> 2, ctx.clip_x2); // screen x2
    i32 yb = min_i32(max3_i32(v20.y, v12.y, v01.y) >> 2, ctx.clip_y2); // screen y2
    i32 xm = (xa << 2) + 2;                                            // mid point top left
    i32 ym = (ya << 2) + 2;
    i32 u0 = v12.y * (xm - vv[1].x) - v12.x * (ym - vv[1].y); // midpoint for subpixel sampling
    i32 v0 = v20.y * (xm - vv[2].x) - v20.x * (ym - vv[2].y);
    i32 w0 = v01.y * (xm - vv[0].x) - v01.x * (ym - vv[0].y);
    i32 ux = v12.y << 2;
    i32 uy = v12.x << 2;
    i32 vx = v20.y << 2;
    i32 vy = v20.x << 2;
    i32 wx = v01.y << 2;
    i32 wy = v01.x << 2;

    for (i32 y = ya; y <= yb; y++) {
        i32 u  = u0;
        i32 v  = v0;
        i32 w  = w0;
        i32 x1 = 0xFFFF;
        i32 x2 = 0;
        for (i32 x = xa; x <= xb; x++) {
            if (0 <= (u | v | w)) {
                x1 = min_i32(x1, x);
                x2 = max_i32(x2, x);
            }
            u += ux;
            v += vx;
            w += wx;
        }

        span_blit_s i = span_blit_gen(ctx, y, x1, x2, mode);
        prim_blit_span(i);
        u0 -= uy;
        v0 -= vy;
        w0 -= wy;
    }
}

void gfx_cir_fill(gfx_ctx_s ctx, v2_i32 p, i32 d, i32 mode)
{
    if (d <= 0) return;

    i32 r = d >> 1;
    if (d <= 2) {
        rec_i32 rec = {p.x - r, p.y - r, d, d};
        gfx_rec_fill(ctx, rec, mode);
        return;
    }

    rec_i32 rb = {p.x - r - 1, p.y - r - 1, d + 1, d + 1};
    rec_i32 rt = {ctx.clip_x1, ctx.clip_y1,
                  ctx.clip_x2 - ctx.clip_x1, ctx.clip_y2 - ctx.clip_y1};
    if (!overlap_rec(rb, rt)) return;

    i32 e = -d + (d == 3 ? +1 : -1);
    i32 x = r; // radius
    i32 y = 0;
    i32 m = (d & 1 ? +1 : 0);

    while (y <= x) {
        i32 ax0 = max_i32(p.x - x, ctx.clip_x1);
        i32 cx0 = max_i32(p.x - y, ctx.clip_x1);
        i32 ax1 = min_i32(p.x + x + m, ctx.clip_x2);
        i32 cx1 = min_i32(p.x + y + m, ctx.clip_x2);

        if (cx0 <= cx1) {
            i32         cy = p.y + x + m;
            i32         dy = p.y - x;
            span_blit_s sc = span_blit_gen(ctx, cy, cx0, cx1, mode);
            if (ctx.clip_y1 <= cy && cy <= ctx.clip_y2) {
                prim_blit_span(sc);
            }
            if (ctx.clip_y1 <= dy && dy <= ctx.clip_y2) {
                prim_blit_span(span_blit_change_y(sc, dy));
            }
        }
        if (ax0 <= ax1) {
            i32         ay = p.y + y + m;
            i32         by = p.y - y;
            span_blit_s sa = span_blit_gen(ctx, ay, ax0, ax1, mode);
            if (ctx.clip_y1 <= ay && ay <= ctx.clip_y2) {
                prim_blit_span(sa);
            }
            if (ctx.clip_y1 <= by && by <= ctx.clip_y2 && ay != by) {
                prim_blit_span(span_blit_change_y(sa, by));
            }
        }

        e += y << 1;
        y++;
        e += y << 1;

        if (0 <= e) {
            x--;
            e -= x << 2;
        }
    }
}

void gfx_lin(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, i32 mode)
{
    gfx_lin_thick(ctx, a, b, mode, 1);
}

typedef struct {
    ALIGNAS(4)
    u16 x1;
    u16 x2;
} gfx_span_s;

// draws thick lines spanning 512 scanlines or less
void gfx_lin_thick(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, i32 mode, i32 d)
{
    // initialize spans
    gfx_span_s spans[512];
    i32        r    = d >> 1;
    i32        ymin = max_i32(min_i32(a.y, b.y) - r - 1, ctx.clip_y1);
    i32        ymax = min_i32(max_i32(a.y, b.y) + r + 1, ctx.clip_y2);
    i32        ydif = ymax - ymin;

    for (i32 n = 0; n <= ydif; n++) {
        gfx_span_s *s = &spans[n];
        s->x1         = U16_MAX;
        s->x2         = 0;
    }

    i32 m  = (d & 1 ? +1 : 0);
    i32 dx = +abs_i32(b.x - a.x);
    i32 dy = -abs_i32(b.y - a.y);
    i32 sx = a.x < b.x ? +1 : -1;
    i32 sy = a.y < b.y ? +1 : -1;
    i32 er = dx + dy;
    i32 xi = a.x;
    i32 xj = b.x;
    i32 yi = a.y - ymin;
    i32 yj = b.y - ymin;

    // generate spans
    while (1) {
        i32 e = -d - 1;
        i32 x = r; // radius
        i32 y = 0;

        while (y <= x) {
            i32 ay  = yi + y + m;
            i32 cy  = yi + x + m;
            i32 by  = yi - y;
            i32 ky  = yi - x;
            i32 ax0 = xi - x;
            i32 cx0 = xi - y;
            i32 ax1 = xi + x + m;
            i32 cx1 = xi + y + m;

            if (0 <= ay && ay <= ydif) {
                gfx_span_s *s = &spans[ay];
                s->x1         = clamp_i32(s->x1, ctx.clip_x1, ax0);
                s->x2         = clamp_i32(s->x2, ax1, ctx.clip_x2);
            }
            if (0 <= by && by <= ydif) {
                gfx_span_s *s = &spans[by];
                s->x1         = clamp_i32(s->x1, ctx.clip_x1, ax0);
                s->x2         = clamp_i32(s->x2, ax1, ctx.clip_x2);
            }
            if (0 <= cy && cy <= ydif) {
                gfx_span_s *s = &spans[cy];
                s->x1         = clamp_i32(s->x1, ctx.clip_x1, cx0);
                s->x2         = clamp_i32(s->x2, cx1, ctx.clip_x2);
            }
            if (0 <= ky && ky <= ydif) {
                gfx_span_s *s = &spans[ky];
                s->x1         = clamp_i32(s->x1, ctx.clip_x1, cx0);
                s->x2         = clamp_i32(s->x2, cx1, ctx.clip_x2);
            }
            e += y << 1;
            y++;
            e += y << 1;

            if (0 <= e) {
                x--;
                e -= x << 2;
            }
        }
        if (xi == xj && yi == yj) break;
        i32 e2 = er << 1;
        if (e2 >= dy) { er += dy, xi += sx; }
        if (e2 <= dx) { er += dx, yi += sy; }
    }

    // blit spans
    for (i32 n = 0; n <= ydif; n++) {
        gfx_span_s *s = &spans[n];

        if (ctx.clip_x1 <= s->x1 && s->x2 <= ctx.clip_x2 && s->x1 <= s->x2) {
            assert(ctx.clip_x1 <= s->x1);
            assert(s->x2 <= ctx.clip_x2);
            assert(s->x1 <= s->x2);
            prim_blit_span(span_blit_gen(ctx, n + ymin, s->x1, s->x2, mode));
        }
    }
}

void gfx_rec(gfx_ctx_s ctx, rec_i32 r, i32 mode)
{
    NOT_IMPLEMENTED
}

void gfx_tri(gfx_ctx_s ctx, tri_i32 t, i32 mode)
{
    gfx_lin_thick(ctx, t.p[0], t.p[1], mode, 2);
    gfx_lin_thick(ctx, t.p[0], t.p[2], mode, 2);
    gfx_lin_thick(ctx, t.p[1], t.p[2], mode, 2);
}

void gfx_cir(gfx_ctx_s ctx, v2_i32 p, i32 r, i32 mode)
{
    NOT_IMPLEMENTED
}

void gfx_spr_copy(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip)
{
    assert(src.t.fmt == TEX_FMT_MASK);
    assert(ctx.dst.fmt == TEX_FMT_OPAQUE);
    if (flip & SPR_FLIP_X) {
        gfx_spr_sm_copy_fx(ctx, src, pos, flip, 0);
    } else {
        gfx_spr_sm_copy(ctx, src, pos, flip, 0);
    }
}

void gfx_spr(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode)
{
    if (!src.t.px) return;
    if (ctx.dst.fmt == TEX_FMT_OPAQUE) {
        if (src.t.fmt == TEX_FMT_OPAQUE) {
            gfx_spr_d_s(ctx, src, pos, flip, mode);
        } else {
            if (flip & SPR_FLIP_X) {
                gfx_spr_sm_fx(ctx, src, pos, flip, mode);
            } else {
                gfx_spr_sm(ctx, src, pos, flip, mode);
            }
        }

    } else {
        if (flip & SPR_FLIP_X) {
            gfx_spr_dm_sm_fx(ctx, src, pos, flip, mode);
        } else {
            gfx_spr_dm_sm(ctx, src, pos, flip, mode);
        }
    }
}

static inline void spr_blit_tile(u32 *dp, u32 sp, u32 sm)
{
    *dp = (*dp & ~sm) | (sp & sm); // copy
}

// special case: drawing tiles <= 32x32 (aligned on texture) to an opaque target texture
// neither XY flipping nor draw modes
void gfx_spr_tile_32x32(gfx_ctx_s ctx, texrec_s src, v2_i32 pos)
{
    if (!src.t.px) return;
    assert((src.x >> 5) == ((src.x + src.w - 1) >> 5));
    assert(ctx.dst.fmt == TEX_FMT_OPAQUE);
    assert(src.t.fmt == TEX_FMT_MASK);

    // area bounds on canvas [x1/y1, x2/y2)
    i32 x1 = max_i32(pos.x, ctx.clip_x1);             // inclusive
    i32 y1 = max_i32(pos.y, ctx.clip_y1);             // inclusive
    i32 x2 = min_i32(pos.x + src.w - 1, ctx.clip_x2); // inclusive
    i32 y2 = min_i32(pos.y + src.h - 1, ctx.clip_y2); // inclusive
    if (x2 < x1 || y2 < y1) return;

    i32  nb = x2 - x1 + 1;                                    // number of bits in a row
    i32  u1 = src.x - pos.x + x1;                             // first bit index in src row
    i32  v1 = src.y - pos.y + y1;                             //
    i32  od = x1 & 31;                                        // bitoffset in dst
    i32  dm = (od + nb - 1) >> 5;                             // number of touched dst words -1
    u32  ml = bswap32(0xFFFFFFFF >> od);                      // mask to cut off boundary left
    u32  mr = bswap32(0xFFFFFFFF << (31 & -(od + nb)));       // mask to cut off boundary right
    i32  l  = 31 & (u1 - x1);                                 // word left shift amount
    i32  r  = 32 - l;                                         // word rght shift amound
    u32 *dp = &ctx.dst.px[ctx.dst.wword * y1 + ((x1 >> 5))];  // dst pixel words
    u32 *sp = &src.t.px[src.t.wword * v1 + ((u1 >> 5) << 1)]; // src pixel words

    if (dm == 0) { // combine mask left and right into one
        ml &= mr;
    }

    for (i32 y = y1; y <= y2; y++, sp += src.t.wword, dp += ctx.dst.wword) {
        u32 zp = bswap32(*(sp + 0));
        u32 zm = bswap32(*(sp + 1));
        u32 p  = bswap32((u32)((u64)zp << l) | (u32)((u64)zp >> r));
        u32 m  = bswap32((u32)((u64)zm << l) | (u32)((u64)zm >> r));
        spr_blit_tile(dp + 0, p, m & ml);
        if (dm == 0) continue;
        spr_blit_tile(dp + 1, p, m & mr);
    }
}

// counter clockwise filled from a1 to a2
void gfx_fill_circle_segment(gfx_ctx_s ctx, v2_i32 p, i32 r, i32 a1_q18, i32 a2_q18, i32 mode)
{
    i32 y1 = max_i32(ctx.clip_y1, p.y - r);
    i32 y2 = min_i32(ctx.clip_y2, p.y + r);
    i32 x1 = max_i32(ctx.clip_x1, p.x - r);
    i32 x2 = min_i32(ctx.clip_x2, p.x + r);
    u32 r2 = (u32)(r * r);

    // >> 2 -> avoid overflow during multiplication
    v2_i32 a = {sin_q16(a1_q18) >> 4, cos_q16(a1_q18) >> 4};
    v2_i32 b = {sin_q16(a2_q18) >> 4, cos_q16(a2_q18) >> 4};
    i32    w = v2_i32_crs(a, b);

    // blit rectangle strips once the x iterator first entered and then
    // leaves the circle segment
    for (i32 y = y1; y <= y2; y++) {
        b32 inside = 0;
        i32 xi     = 0;

        for (i32 x = x1; x <= x2; x++) {
            v2_i32 s = {x - p.x, y - p.y};
            i32    u = v2_i32_crs(a, s);
            i32    v = v2_i32_crs(b, s);

            if (v2_i32_lensq(s) <= r2 &&
                ((0 <= w && (0 <= u && v <= 0)) ||
                 (0 >= w && (0 <= u || v <= 0)))) {
                if (!inside) {
                    inside = 1;
                    xi     = x;
                }
            } else {
                if (inside) {
                    inside = 0;
                    gfx_rec_strip(ctx, xi, y, x - xi, mode);
                }
            }
        }

        // if still inside draw remaining widest strip up to x2
        if (inside) {
            gfx_rec_strip(ctx, xi, y, x2 - xi, mode);
        }
    }
}

// counter clockwise filled from a1 to a2
void gfx_fill_circle_ring_seg(gfx_ctx_s ctx, v2_i32 p, i32 ri, i32 ro, i32 a1_q17, i32 a2_q17, i32 mode)
{
    i32 y1 = max_i32(ctx.clip_y1, p.y - ro);
    i32 y2 = min_i32(ctx.clip_y2, p.y + ro);
    i32 x1 = max_i32(ctx.clip_x1, p.x - ro);
    i32 x2 = min_i32(ctx.clip_x2, p.x + ro);
    i32 r2 = ro * ro;
    i32 r1 = ri * ri;

    // >> 2 -> avoid overflow during multiplication
    v2_i32 a = {sin_q15(a1_q17) >> 2, cos_q15(a1_q17) >> 2};
    v2_i32 b = {sin_q15(a2_q17) >> 2, cos_q15(a2_q17) >> 2};
    i32    w = v2_i32_crs(a, b);

    // blit rectangle strips once the x iterator first entered and then
    // leaves the circle segment
    for (i32 y = y1; y <= y2; y++) {
        b32 was_inside = 0;
        i32 x1_strip   = 0;

        for (i32 x = x1; x <= x2; x++) {
            v2_i32 s = {x - p.x, y - p.y};
            i32    r = v2_i32_lensq(s);
            b32    i = 0;

            if (r1 <= r && r <= r2) {
                i32 u = v2_i32_crs(a, s);
                i32 v = v2_i32_crs(b, s);
                i     = (0 <= w && (0 <= u && v <= 0)) ||
                    (0 >= w && (0 <= u || v <= 0));
            }

            if (i && !was_inside) {
                x1_strip = x;
            } else if (!i && was_inside) {
                gfx_rec_strip(ctx, x1_strip, y, x - x1_strip, mode);
            }
            was_inside = i;
        }
        if (was_inside) {
            gfx_rec_strip(ctx, x1_strip, y, x2 - x1_strip, mode);
        }
    }
}

ALIGNAS(32)
static const u32 g_bayer_8x8[65 * 8] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x80808080, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x80808080, 0x00000000, 0x00000000, 0x00000000,
    0x08080808, 0x00000000, 0x00000000, 0x00000000,
    0x88888888, 0x00000000, 0x00000000, 0x00000000,
    0x08080808, 0x00000000, 0x00000000, 0x00000000,
    0x88888888, 0x00000000, 0x00000000, 0x00000000,
    0x88888888, 0x00000000, 0x00000000, 0x00000000,
    0x88888888, 0x00000000, 0x20202020, 0x00000000,
    0x88888888, 0x00000000, 0x00000000, 0x00000000,
    0x88888888, 0x00000000, 0x20202020, 0x00000000,
    0x88888888, 0x00000000, 0x02020202, 0x00000000,
    0x88888888, 0x00000000, 0x22222222, 0x00000000,
    0x88888888, 0x00000000, 0x02020202, 0x00000000,
    0x88888888, 0x00000000, 0x22222222, 0x00000000,
    0x88888888, 0x00000000, 0x22222222, 0x00000000,
    0xA8A8A8A8, 0x00000000, 0x22222222, 0x00000000,
    0x88888888, 0x00000000, 0x22222222, 0x00000000,
    0xA8A8A8A8, 0x00000000, 0x22222222, 0x00000000,
    0x8A8A8A8A, 0x00000000, 0x22222222, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0x22222222, 0x00000000,
    0x8A8A8A8A, 0x00000000, 0x22222222, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0x22222222, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0x22222222, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0xA2A2A2A2, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0x22222222, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0xA2A2A2A2, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0x2A2A2A2A, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0x2A2A2A2A, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x40404040, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x00000000, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x40404040, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x04040404, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x04040404, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x10101010,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x00000000,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x10101010,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x01010101,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x01010101,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x54545454, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x54545454, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x45454545, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x45454545, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x51515151,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x11111111,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x51515151,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x15151515,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x15151515,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xEAEAEAEA, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xEAEAEAEA, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xAEAEAEAE, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xAEAEAEAE, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xBABABABA, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xAAAAAAAA, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xBABABABA, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xABABABAB, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xABABABAB, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xFEFEFEFE, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xEEEEEEEE, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xFEFEFEFE, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xEFEFEFEF, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xEFEFEFEF, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xFBFBFBFB, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xBBBBBBBB, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xFBFBFBFB, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xBFBFBFBF, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xBFBFBFBF, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0xD5D5D5D5, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0x55555555, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0xD5D5D5D5, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0x5D5D5D5D, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0x5D5D5D5D, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x75757575,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x55555555,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x75757575,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x57575757,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x57575757,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xFDFDFDFD, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xFDFDFDFD, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xDFDFDFDF, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xDFDFDFDF, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xF7F7F7F7,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x77777777,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xF7F7F7F7,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x7F7F7F7F,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x7F7F7F7F,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};