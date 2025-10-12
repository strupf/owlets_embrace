// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/gfx.h"
#include "core/spm.h"
#include "pltf/pltf.h"
#include "util/json.h"
#include "util/mathfunc.h"
#include "util/sorting.h"
#include "util/str.h"

#define SPRBLIT_FUNCNAME gfx_spr_d_s
#define SPRBLIT_SRC_MASK 0
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 0
#include "core/gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_dm_sm
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 1
#define SPRBLIT_FLIPPEDX 0
#include "core/gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_sm
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 0
#include "core/gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_dm_sm_fx
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 1
#define SPRBLIT_FLIPPEDX 1
#include "core/gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_sm_fx
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 1
#include "core/gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_sm_fx_copy
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 1
#define SPRBLIT_COPYMODE 1
#include "core/gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX
#undef SPRBLIT_COPYMODE

#define SPRBLIT_FUNCNAME gfx_spr_sm_copy
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 0
#define SPRBLIT_COPYMODE 1
#include "core/gfx_spr_func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX
#undef SPRBLIT_COPYMODE

extern const u32 g_bayer_8x8[65 * 8];

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
    if (!((u32)x < (u32)tex.w && (u32)y < (u32)tex.h)) return 0;
    return tex_px_at_unsafe(tex, x, y);
}

i32 tex_mk_at(tex_s tex, i32 x, i32 y)
{
    if (!((u32)x < (u32)tex.w && (u32)y < (u32)tex.h)) return 1;
    return tex_mk_at_unsafe(tex, x, y);
}

void tex_px(tex_s tex, i32 x, i32 y, i32 col)
{
    if ((u32)x < (u32)tex.w && (u32)y < (u32)tex.h) {
        tex_px_unsafe(tex, x, y, col);
    }
}

void tex_mk(tex_s tex, i32 x, i32 y, i32 col)
{
    if ((u32)x < (u32)tex.w && (u32)y < (u32)tex.h) {
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

            if (sm == 0xFFFFFFFFU) { // nothing to outline in this word, fully opaque
                *d = sp;
            } else {
                u32 m = 0;
                for (i32 yi = yi1; yi <= yi2; yi++) {
                    i32 v = 1 + yi * src.wword;
                    u32 k = *(s + v); // top, bot and current word
                    if (k) {
                        k = bswap32(k);
                        m |= k | (k >> 1) | (k << 1);
                    }

#if 0 // use bswap?
                    if (0 <= (x - 2)) {
                        m |= bswap32(*(s + v - 2)) << 31; // neighbors left top, left, left bot
                    }
                    if ((x + 2) < src.wword) {
                        m |= bswap32(*(s + v + 2)) >> 31; // neighbors right top, right, right bot
                    }
#else
                    if (0 <= (x - 2)) {
                        m |= (*(s + v - 2) << 7) & (u32)0x80000000; // neighbors left top, left, left bot
                    }
                    if ((x + 2) < src.wword) {
                        m |= (*(s + v + 2) >> 6) & (u32)0x00000001; // neighbors right top, right, right bot
                    }
#endif
                }
                if (m) {
                    m = bswap32(m) & ~sm; // white outline only on transparency
                    sp |= m;
                    sm |= m;
                    *d = (*d & ~sm) | (sp & sm); // copy mode
                }
            }
            d += 1;
            s += 2;
        }
    }
}

gfx_ctx_s gfx_ctx_from_tex(tex_s dst)
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
    return gfx_ctx_from_tex(tex_framebuffer());
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
    if (!dst.px) return;

    u32 *p     = dst.px;
    u32 *p_end = dst.px + dst.wword * dst.h;

    switch (col) {
    case GFX_COL_BLACK:
        switch (dst.fmt) {
        case TEX_FMT_OPAQUE:
            mclr(p, (i32)((byte *)p_end - (byte *)p));
            break;
        case TEX_FMT_MASK:
            while (p < p_end) {
                *p++ = 0;          // data
                *p++ = 0xFFFFFFFF; // mask
            }
            break;
        }
        break;
    case GFX_COL_WHITE:
        switch (dst.fmt) {
        case TEX_FMT_OPAQUE:
            mset(p, 0xFF, (i32)((byte *)p_end - (byte *)p));
            break;
        case TEX_FMT_MASK:
            mset(p, 0xFF, (i32)((byte *)p_end - (byte *)p));
            break;
        }
        break;
    case GFX_COL_CLEAR:
        if (dst.fmt == TEX_FMT_OPAQUE) break;

#if 1
        for (p++; p < p_end; p += 2) {
            *p = 0; // mask
        }
#else
        mclr(p, (i32)((byte *)p_end - (byte *)p));
#endif
        break;
    }
}

usize tex_size_bytes(tex_s t)
{
    return (sizeof(u32) * t.wword * t.h);
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
            gfx_spr_sm_copy(ctx, src, p, SPR_FLIP_X, 0);
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

typedef struct {
    ALIGNAS(4)
    u16 x1;
    u16 x2;
} gfx_span_s;

void gfx_lin(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, i32 mode)
{
    gfx_lin_thick(ctx, a, b, mode, 1);
}

// draws thick lines with diameter d
// currently only hard caps
void gfx_lin_thick(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, i32 mode, i32 d)
{
    if (a.x == b.x) { // vertical, rectangle
        rec_i32 r = {a.x - (d >> 1), min_i32(a.y, b.y), d, abs_i32(b.y - a.y)};
        gfx_rec_fill(ctx, r, mode);
        return;
    }
    if (a.y == b.y) { // horizontal, rectangle
        rec_i32 r = {min_i32(a.x, b.x), a.y - (d >> 1), abs_i32(b.x - a.x), d};
        gfx_rec_fill(ctx, r, mode);
        return;
    }

#define GFX_LIN_THICK_SUBPX 4

    // create a rotated rectangle out of 4 lines (4 points)
    // start by offseting the input line parallel by half the diameter
    // scaled up by a factor for increased precision
    v2_i32 ap     = v2_i32_mul(a, GFX_LIN_THICK_SUBPX);
    v2_i32 bp     = v2_i32_mul(b, GFX_LIN_THICK_SUBPX);
    v2_i32 v_orth = {-(bp.y - ap.y), +(bp.x - ap.x)};
    v_orth        = v2_i32_setlen(v_orth, d * GFX_LIN_THICK_SUBPX);

    ALIGNAS(32) v2_i32 pt[4];
    {
        v2_i32 p0 = {ap.x - (v_orth.x / 2), ap.y - (v_orth.y / 2)};
        v2_i32 p1 = {bp.x - (v_orth.x / 2), bp.y - (v_orth.y / 2)};
        v2_i32 p2 = {p1.x + v_orth.x, p1.y + v_orth.y};
        v2_i32 p3 = {p0.x + v_orth.x, p0.y + v_orth.y};
        pt[0]     = p0;
        pt[1]     = p1;
        pt[2]     = p2;
        pt[3]     = p3;
    }

    i32 y_min = I32_MAX;
    i32 y_max = I32_MIN;
    for (i32 n = 0; n < 4; n++) {
        v2_i32 p = pt[n];
        y_min    = min_i32(y_min, p.y);
        y_max    = max_i32(y_max, p.y);
    }
    y_min = max_i32(y_min / GFX_LIN_THICK_SUBPX - 1, ctx.clip_y1);
    y_max = min_i32(y_max / GFX_LIN_THICK_SUBPX + 1, ctx.clip_y2);

    for (i32 yi_px = y_min; yi_px <= y_max; yi_px++) {
        i32 x1 = I32_MAX;
        i32 x2 = I32_MIN;
        i32 yi = yi_px * GFX_LIN_THICK_SUBPX + (GFX_LIN_THICK_SUBPX / 2);

        for (i32 n = 0; n < 4; n++) {
            v2_i32 p0 = pt[n];
            v2_i32 p1 = pt[(n + 1) & 3];

            i32 num;
            i32 den;
            ratio_along(p0.y, p1.y, yi, &num, &den);

            if (den && ((0 <= num && num <= den) || (den <= num && num <= 0))) {
                i32 xi = p0.x + div_rounded_i32((p1.x - p0.x) * num, den);
                x1     = min_i32(x1, xi);
                x2     = max_i32(x2, xi);
            }
        }

        x1 = max_i32(x1 / GFX_LIN_THICK_SUBPX, ctx.clip_x1);
        x2 = min_i32(x2 / GFX_LIN_THICK_SUBPX, ctx.clip_x2);
        if (x1 <= x2) {
            prim_blit_span(span_blit_gen(ctx, yi_px, x1, x2, mode));
        }
    }
}

void gfx_rec(gfx_ctx_s ctx, rec_i32 r, i32 mode)
{
    NOT_IMPLEMENTED("gfx rectangle outline");
}

void gfx_tri(gfx_ctx_s ctx, tri_i32 t, i32 mode)
{
    gfx_lin_thick(ctx, t.p[0], t.p[1], mode, 2);
    gfx_lin_thick(ctx, t.p[0], t.p[2], mode, 2);
    gfx_lin_thick(ctx, t.p[1], t.p[2], mode, 2);
}

void gfx_cir(gfx_ctx_s ctx, v2_i32 p, i32 r, i32 mode)
{
    NOT_IMPLEMENTED("gfx cicle outline");
}

void gfx_spr_copy(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip)
{
    assert(src.t.fmt == TEX_FMT_MASK);
    assert(ctx.dst.fmt == TEX_FMT_OPAQUE);
    if (flip & SPR_FLIP_X) {
        gfx_spr_sm_fx_copy(ctx, src, pos, flip, 0);
    } else {
        gfx_spr_sm_fx(ctx, src, pos, flip, 0);
    }
}

void gfx_spr2(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode)
{
    i32 x1 = max_i32(ctx.clip_x1, pos.x);
    i32 y1 = max_i32(ctx.clip_y1, pos.y);
    i32 x2 = min_i32(ctx.clip_x2, pos.x + src.w - 1);
    i32 y2 = min_i32(ctx.clip_y2, pos.y + src.h - 1);
    if (x2 < x1 || y2 < y1) return;

    tex_s t_d = ctx.dst;
    tex_s t_s = src.t;
    i32   dw1 = x1 >> 5;
    i32   dw2 = x2 >> 5;
    i32   o_x = pos.x - src.x;
    i32   shr = o_x & 31;
    i32   shl = 32 - shr;
    u32   c_l = bswap32(0xFFFFFFFF >> (x1 & 31));
    u32   c_r = bswap32(0xFFFFFFFF << (31 - (x2 & 31)));
    i32   s_y = flip & SPR_FLIP_Y ? -1 : +1;
    i32   a_y = 0 < s_y ? src.y - pos.y : src.y + pos.y + src.h - 1;

    for (i32 y_d = y1; y_d <= y2; y_d++) {
        i32 y_s = s_y * y_d + a_y;
        u32 pat = ctx.pat.p[y_d & 7];
        u32 c_m = c_l; // clipping word (first dst word is left clipped)

        for (i32 d_w = dw1; d_w <= dw2; d_w++, c_m = 0xFFFFFFFF) {
            i32 sx1 = max_i32((d_w << 5) - o_x + 0, 0);          // left most pixel of source for this destination word
            i32 sx2 = min_i32((d_w << 5) - o_x + 31, t_s.w - 1); // right most pixel of source for this destination word

            if (d_w == dw2) {
                c_m &= c_r; // right clipped
            }

            u32  sm1 = bswap32(t_s.px[((sx1 >> 5) << 1) + 1 + y_s * t_s.wword]);
            u32  sm2 = bswap32(t_s.px[((sx2 >> 5) << 1) + 1 + y_s * t_s.wword]);
            u32  sp1 = bswap32(t_s.px[((sx1 >> 5) << 1) + 0 + y_s * t_s.wword]);
            u32  sp2 = bswap32(t_s.px[((sx2 >> 5) << 1) + 0 + y_s * t_s.wword]);
            u32  smm = (u32)((u64)sm1 << shl) | (u32)((u64)sm2 >> shr);
            u32  spp = (u32)((u64)sp1 << shl) | (u32)((u64)sp2 >> shr);
            u32 *dpp = &t_d.px[d_w + y_d * t_d.wword];
            spr_blit_p(dpp, bswap32(spp), bswap32(smm) & c_m, pat, mode);
        }
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
                // gfx_spr_blit_new(ctx, src, pos, flip, mode);
            } else {
                gfx_spr_sm(ctx, src, pos, flip, mode);
                // gfx_spr2(ctx, src, pos, flip, mode);
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
    assert(src.t.px);
    assert((src.x >> 5) == ((src.x + src.w - 1) >> 5));
    assert(ctx.dst.fmt == TEX_FMT_OPAQUE);
    assert(src.t.fmt == TEX_FMT_MASK);

    // area bounds on canvas [x1/y1, x2/y2)
    i32 x1 = max_i32(pos.x, ctx.clip_x1);
    i32 y1 = max_i32(pos.y, ctx.clip_y1);
    i32 x2 = min_i32(pos.x + src.w - 1, ctx.clip_x2);
    i32 y2 = min_i32(pos.y + src.h - 1, ctx.clip_y2);
    if (x2 < x1 || y2 < y1) return; // clip, not visible

    i32  nb = x2 - x1 + 1;                                    // number of bits in a row
    i32  od = x1 & 31;                                        // bitoffset in dst
    i32  dm = (od + nb - 1) >> 5;                             // number of touched dst words -1
    u32  ml = bswap32(0xFFFFFFFF >> od);                      // mask to cut off boundary left
    u32  mr = bswap32(0xFFFFFFFF << (31 & -(od + nb)));       // mask to cut off boundary right
    i32  u1 = src.x - pos.x + x1;                             // first bit index in src row
    i32  v1 = src.y - pos.y + y1;                             // first row index
    i32  l  = 31 & (u1 - x1);                                 // word left shift amount
    i32  r  = 32 - l;                                         // word rght shift amound
    u32 *dp = &ctx.dst.px[ctx.dst.wword * y1 + ((x1 >> 5))];  // dst pixel words
    u32 *sp = &src.t.px[src.t.wword * v1 + ((u1 >> 5) << 1)]; // src pixel words

    if (dm == 0) { // combine mask left and right if only one destination word is affected
        ml &= mr;
    }

    u32 *sp_end_incl = sp + (y2 - y1) * src.t.wword;
    do {
        u32 zp = bswap32(*(sp + 0));
        u32 zm = bswap32(*(sp + 1));
        u32 p  = bswap32((u32)((u64)zp << l) | (u32)((u64)zp >> r));
        u32 m  = bswap32((u32)((u64)zm << l) | (u32)((u64)zm >> r));
        {
            spr_blit_tile(dp + 0, p, m & ml); // blit to 1st destination word
        }
        if (dm) {
            spr_blit_tile(dp + 1, p, m & mr); // blit to 2nd destination word (if needed)
        }
        sp += src.t.wword;
        dp += ctx.dst.wword;
    } while (sp <= sp_end_incl);
}

static void gfx_fill_circle_segment_span(gfx_ctx_s ctx, i32 y, i32 x1, i32 x2, v2_i32 p, v2_i32 a, v2_i32 b, i32 w, i32 mode)
{
    b32 inside = 0;
    i32 xi     = 0;

    for (i32 x = x1; x <= x2; x++) {
        v2_i32 s = {x - p.x, y - p.y};
        i32    u = v2_i32_crs(a, s);
        i32    v = v2_i32_crs(b, s);

        if ((0 <= w && (0 <= u && v <= 0)) ||
            (0 >= w && (0 <= u || v <= 0))) {
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

// counter clockwise filled from a1 to a2
void gfx_fill_circle_segment(gfx_ctx_s ctx, v2_i32 p, i32 d, i32 a1_q18, i32 a2_q18, i32 mode)
{
    if (d <= 2) return;

    i32     r  = d >> 1;
    rec_i32 rb = {p.x - r - 1, p.y - r - 1, d + 1, d + 1};
    rec_i32 rt = {ctx.clip_x1, ctx.clip_y1, ctx.clip_x2 - ctx.clip_x1, ctx.clip_y2 - ctx.clip_y1};
    if (!overlap_rec(rb, rt)) return;

    i32 e = -d + (d == 3 ? +1 : -1);
    i32 x = r; // radius
    i32 y = 0;
    i32 m = (d & 1 ? +1 : 0);

    // >> 2 -> avoid overflow during multiplication
    v2_i32 a = {sin_q16(a1_q18) >> 4, cos_q16(a1_q18) >> 4};
    v2_i32 b = {sin_q16(a2_q18) >> 4, cos_q16(a2_q18) >> 4};
    i32    w = v2_i32_crs(a, b);

    while (y <= x) {
        i32 ax0 = max_i32(p.x - x, ctx.clip_x1);
        i32 cx0 = max_i32(p.x - y, ctx.clip_x1);
        i32 ax1 = min_i32(p.x + x + m, ctx.clip_x2);
        i32 cx1 = min_i32(p.x + y + m, ctx.clip_x2);

        if (cx0 <= cx1) {
            i32 cy = p.y + x + m;
            i32 dy = p.y - x;
            if (ctx.clip_y1 <= cy && cy <= ctx.clip_y2) {
                gfx_fill_circle_segment_span(ctx, cy, cx0, cx1, p, a, b, w, mode);
            }
            if (ctx.clip_y1 <= dy && dy <= ctx.clip_y2) {
                gfx_fill_circle_segment_span(ctx, dy, cx0, cx1, p, a, b, w, mode);
            }
        }
        if (ax0 <= ax1) {
            i32 ay = p.y + y + m;
            i32 by = p.y - y;
            if (ctx.clip_y1 <= ay && ay <= ctx.clip_y2) {
                gfx_fill_circle_segment_span(ctx, ay, ax0, ax1, p, a, b, w, mode);
            }
            if (ctx.clip_y1 <= by && by <= ctx.clip_y2 && ay != by) {
                gfx_fill_circle_segment_span(ctx, by, ax0, ax1, p, a, b, w, mode);
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
                i     = (0 <= w && (0 <= u && v <= 0)) || (0 >= w && (0 <= u || v <= 0));
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
const u32 g_bayer_8x8[65 * 8] = {
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