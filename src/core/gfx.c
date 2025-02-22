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

tex_s tex_create(i32 w, i32 h, b32 mask, allocator_s a, i32 *err)
{
    tex_s t         = {0};
    b32   m         = mask != 0;
    u32   waligned  = (w + 31) & ~31;
    u32   wword     = (waligned >> 5) << (i32)m;
    u32   size      = sizeof(u32) * wword * h;
    u32   alignment = 4 << m;
    void *mem       = a.allocfunc(a.ctx, size, alignment);
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

static i32 tex_mk_at_unsafe(tex_s tex, i32 x, i32 y)
{
    if (tex.fmt == TEX_FMT_OPAQUE) return 1;

    u32 b = bswap32(0x80000000 >> (x & 31));
    return (tex.px[y * tex.wword + ((x >> 5) << 1) + 1] & b);
}

static void tex_px_unsafe(tex_s tex, i32 x, i32 y, i32 col)
{
    u32  b = bswap32(0x80000000 >> (x & 31));
    u32 *p = NULL;
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

void tex_outline(tex_s tex, i32 x, i32 y, i32 w, i32 h, i32 col, bool32 dia)
{
    spm_push();
    tex_s src  = tex; // need to work off of a copy
    u32   size = sizeof(u32) * tex.wword * tex.h;
    src.px     = (u32 *)spm_alloc_aligned(size, 8);
    mcpy(src.px, tex.px, size);

    i32 x2 = x + w;
    i32 y2 = y + h;
    for (i32 yy = y; yy < y2; yy++) {
        for (i32 xx = x; xx < x2; xx++) {

            if (tex_mk_at_unsafe(src, xx, yy)) continue;

            for (i32 u = -1; u <= +1; u++) {
                for (i32 v = -1; v <= +1; v++) {
                    if ((u | v) == 0) continue; // same pixel
                    if (!dia && v != 0 && u != 0) continue;

                    i32 t = xx + u;
                    i32 s = yy + v;
                    if (!(x <= t && t < x2 && y <= s && s < y2)) continue;

                    if (tex_mk_at_unsafe(src, t, s)) {
                        tex_mk_unsafe(tex, xx, yy, 1);
                        tex_px_unsafe(tex, xx, yy, col);
                        goto BREAK_LOOP;
                    }
                }
            }
        BREAK_LOOP:;
        }
    }
    spm_pop();
}

// only works if x and w are multiple of 32 right now
void tex_outline_white(tex_s tex)
{
    assert(tex.fmt == TEX_FMT_MASK);
    spm_push();
    tex_s src  = tex; // need to work off of a copy
    u32   size = sizeof(u32) * tex.wword * tex.h;
    src.px     = (u32 *)spm_alloc_aligned(size, 4);
    mcpy(src.px, tex.px, size);

    i32 x1 = 0;
    i32 y1 = 0;
    i32 x2 = tex.wword - 1;
    i32 y2 = tex.h - 1;

    for (i32 yy = y1; yy <= y2; yy++) {
        i32 yi1 = yy == y1 ? 0 : -1;
        i32 yi2 = yy == y2 ? 0 : +1;

        for (i32 xx = x1; xx <= x2; xx += 2) {
            u32 *dp = &tex.px[xx + (yy * src.wword)];
            u32 *dm = dp + 1;
            u32  m  = 0;

            for (i32 yi = yi1; yi <= yi2; yi++) {
                i32 v  = yy + yi;
                i32 ua = xx - 2;
                i32 ub = xx + 0;
                i32 uc = xx + 2;
                if (x1 <= ua && ua <= x2) {
                    u32 k = bswap32(src.px[ua + v * src.wword + 1]);
                    m |= k << 31;
                }
                if (x1 <= ub && ub <= x2) {
                    u32 k = bswap32(src.px[ub + v * src.wword + 1]);
                    m |= (k >> 1) | (k << 1);
                }
                if (x1 <= uc && uc <= x2) {
                    u32 k = bswap32(src.px[uc + v * src.wword + 1]);
                    m |= k >> 31;
                }
            }
            m = bswap32(m) & ~*dm;
            *dm |= m;
            *dp |= m;
        }
    }
    spm_pop();
}

void tex_merge_to_opaque(tex_s dst, tex_s src)
{
    assert(dst.fmt == TEX_FMT_OPAQUE);
    assert(src.fmt == TEX_FMT_MASK);
    assert(dst.wword * 2 == src.wword && dst.h == src.h);

    i32  nw = dst.wword * dst.h;
    u32 *a  = dst.px;
    u32 *b  = src.px;
    for (i32 n = 0; n < nw; n++) {
        *a = (*a & ~b[1]) | (b[0] & b[1]); // copy mode
        a += 1;
        b += 2;
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
        i32 yi1 = y + (y == 0 ? 0 : -1);
        i32 yi2 = y + (y == src.h - 1 ? 0 : +1);

        for (i32 x = 0; x < src.wword; x += 2, d += 1) {
            u32 sp = s[x + (y * src.wword)];
            u32 sm = s[x + (y * src.wword) + 1];
            if (sm != 0xFFFFFFFF) {
                u32 m = 0;

                for (i32 yi = yi1; yi <= yi2; yi++) {
                    i32 v  = 1 + yi * src.wword;
                    i32 wl = x - 2;
                    i32 wr = x + 2;
                    u32 k  = bswap32(s[x + v]);
                    m |= (k >> 1) | (k << 1);
                    if (0 <= wl) {
                        m |= bswap32(s[wl + v]) << 31;
                    }
                    if (wr < src.wword) {
                        m |= bswap32(s[wr + v]) >> 31;
                    }
                }

                m = bswap32(m) & ~sm; // white outline only on transparency
                sp |= m;              // add white outline
                sm |= m;              // add white outline
            }

            *d = (*d & ~sm) | (sp & sm); // copy mode
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
    gfx_pattern_s r = {{~p.p[0], ~p.p[1], ~p.p[2], ~p.p[3],
                        ~p.p[4], ~p.p[5], ~p.p[6], ~p.p[7]}};
    return r;
}

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
        u32 pa       = ((u32)p[i] << 4) | ((u32)p[i]);
        u32 pb       = (pa << 24) | (pa << 16) | (pa << 8) | (pa);
        pat.p[i + 0] = pb;
        pat.p[i + 4] = pb;
    }
    return pat;
}

gfx_pattern_s gfx_pattern_8x8(i32 p0, i32 p1, i32 p2, i32 p3,
                              i32 p4, i32 p5, i32 p6, i32 p7)
{
    gfx_pattern_s pat  = {0};
    u32           p[8] = {p0, p1, p2, p3, p4, p5, p6, p7};
    for (i32 i = 0; i < 8; i++) {
        pat.p[i] = (p[i] << 24) | (p[i] << 16) | (p[i] << 8) | (p[i]);
    }
    return pat;
}

gfx_pattern_s gfx_pattern_bayer_4x4(i32 i)
{
    static const u32 ditherpat[GFX_PATTERN_NUM * 4] = {
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x88888888, 0x00000000, 0x00000000, 0x00000000,
        0x88888888, 0x00000000, 0x22222222, 0x00000000,
        0xAAAAAAAA, 0x00000000, 0x22222222, 0x00000000,
        0xAAAAAAAA, 0x00000000, 0xAAAAAAAA, 0x00000000,
        0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x00000000,
        0xAAAAAAAA, 0x44444444, 0xAAAAAAAA, 0x11111111,
        0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x11111111,
        0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555,
        0xEEEEEEEE, 0x55555555, 0xAAAAAAAA, 0x55555555,
        0xEEEEEEEE, 0x55555555, 0xBBBBBBBB, 0x55555555,
        0xFFFFFFFF, 0x55555555, 0xBBBBBBBB, 0x55555555,
        0xFFFFFFFF, 0x55555555, 0xFFFFFFFF, 0x55555555,
        0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x55555555,
        0xFFFFFFFF, 0xDDDDDDDD, 0xFFFFFFFF, 0x77777777,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x77777777,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

    const u32    *p   = &ditherpat[clamp_i32(i, 0, GFX_PATTERN_MAX) << 2];
    gfx_pattern_s pat = {{p[0], p[1], p[2], p[3], p[0], p[1], p[2], p[3]}};
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

gfx_pattern_s gfx_pattern_interpolatec(i32 num, i32 den,
                                       i32 (*ease)(i32 a, i32 b, i32 num, i32 den))
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

// used for simple sprites
typedef struct {
    u32          *dp;   // pixel
    u16           dmax; // count of dst words -1
    u16           dadd;
    u32           ml;   // boundary mask left
    u32           mr;   // boundary mask right
    i16           mode; // drawing mode
    i16           doff; // bitoffset of first dst bit
    u16           dst_wword;
    i16           y;
    gfx_pattern_s pat;
} span_blit_s;

static span_blit_s span_blit_gen(gfx_ctx_s ctx, i32 y, i32 x1, i32 x2, i32 mode)
{
#if 0
    i32         nbit = (x2 + 1) - x1; // number of bits in a row to blit
    i32         lsh  = (ctx.dst.fmt == TEX_FMT_MASK);
    span_blit_s i    = {0};
    i.y              = y;
    i.doff           = x1 & 31;
    i.dmax           = (i.doff + nbit - 1) >> 5;                       // number of touched dst words -1
    i.mode           = mode;                                           // sprite masking mode
    i.ml             = bswap32(0xFFFFFFFF >> (31 & i.doff));           // mask to cut off boundary left
    i.mr             = bswap32(0xFFFFFFFF << (31 & (-i.doff - nbit))); // mask to cut off boundary right
    if (i.dmax == 0) {
        i.ml &= i.mr;
    }
    i.dst_wword = ctx.dst.wword;
    i.dp        = &ctx.dst.px[((x1 >> 5) << lsh) + y * ctx.dst.wword];
    i.dadd      = 1 + lsh;
    i.pat       = ctx.pat;
    return i;
#else
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
#endif
}

static inline void span_blit_incr_y(span_blit_s *info)
{
    info->y++;
    info->dp += info->dst_wword;
}

void gfx_spr_tiled(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode, i32 tx, i32 ty)
{
    if (!src.t.px) return;
    if ((src.w | src.h) == 0) return;
    i32 dx = tx ? tx : src.w;
    i32 dy = ty ? ty : src.h;
    i32 x1 = tx ? ((pos.x % dx) - dx) % dx : pos.x;
    i32 y1 = ty ? ((pos.y % dy) - dy) % dy : pos.y;
    i32 x2 = tx ? ((ctx.dst.w - x1) / dx) * dx : x1;
    i32 y2 = ty ? ((ctx.dst.h - y1) / dy) * dy : y1;

    for (i32 y = y1; y <= y2; y += dy) {
        for (i32 x = x1; x <= x2; x += dx) {
            v2_i32 p = {x, y};
            gfx_spr(ctx, src, p, flip, mode);
        }
    }
}

void gfx_spr_tileds(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode, bool32 x, bool32 y)
{
    gfx_spr_tiled(ctx, src, pos, flip, mode, x ? src.w : 0, y ? src.h : 0);
}

void gfx_spr_rotscl(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, v2_i32 origin, f32 angle,
                    f32 sclx, f32 scly)
{
    tex_s dst = ctx.dst;
    origin.x += src.x;
    origin.y += src.y;
    m33_f32 m1 = m33_offset(+(f32)origin.x, +(f32)origin.y);
    m33_f32 m3 = m33_offset(-(f32)pos.x - (f32)origin.x, -(f32)pos.y - (f32)origin.y);
    m33_f32 m4 = m33_rotate(angle);
    m33_f32 m5 = m33_scale(sclx, scly);

    m33_f32 m = m33_identity();
    m         = m33_mul(m, m3);
    m         = m33_mul(m, m5);
    m         = m33_mul(m, m4);
    m         = m33_mul(m, m1);

    i32 x1 = 0; // optimize by inverting the matrix and calc corners
    i32 y1 = 0;
    i32 x2 = ctx.dst.w - 1;
    i32 y2 = ctx.dst.h - 1;
    i32 w1 = x1 >> 5;
    i32 w2 = x2 >> 5;

    f32 u1 = (f32)(src.x);
    f32 v1 = (f32)(src.y);
    f32 u2 = (f32)(src.x + src.w);
    f32 v2 = (f32)(src.y + src.h);

    for (i32 y = y1; y <= y2; y++) {
        u32 pat = ctx.pat.p[y & 7];
        for (i32 wi = w1; wi <= w2; wi++) {

            i32 p1 = wi == w1 ? x1 & 31 : 0;
            i32 p2 = wi == w2 ? x2 & 31 : 31;
            u32 sp = 0;
            u32 sm = 0;

            for (i32 p = p1; p <= p2; p++) {
                i32 x = (wi << 5) + p;

                f32 t = (f32)(x + src.x);
                f32 s = (f32)(y + src.y);
                i32 u = (i32)(t * m.m[0] + s * m.m[1] + m.m[2] + .5f);
                i32 v = (i32)(t * m.m[3] + s * m.m[4] + m.m[5] + .5f);
                if (!(u1 <= u && u < u2)) continue;
                if (!(v1 <= v && v < v2)) continue;
                u32 bit = 0x80000000 >> p;

                if (tex_px_at_unsafe(src.t, u, v)) sp |= bit;
                if (tex_mk_at_unsafe(src.t, u, v)) sm |= bit;
            }

            u32 *dp = 0;
            u32 *dm = 0;
            switch (ctx.dst.fmt) {
            case TEX_FMT_OPAQUE:
                dp = &ctx.dst.px[y * dst.wword + wi];
                break;
            case TEX_FMT_MASK:
                dp = &dst.px[y * dst.wword + (wi << 1)];
                dm = dp + 1;
                break;
            default: BAD_PATH
            }

            if (dm) {
                spr_blit_pm(dp, dm, bswap32(sp), bswap32(sm) & pat, 0);
            } else {
                spr_blit_p(dp, bswap32(sp), bswap32(sm) & pat, 0);
            }
        }
    }
}

void gfx_spr_rotated(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, v2_i32 origin, f32 angle)
{
    gfx_spr_rotscl(ctx, src, pos, origin, angle, 1.f, 1.f);
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
    assert(0 <= y1 && y2 <= dst.h);
    u32 *px = &dst.px[y1 * dst.wword];
    for (i32 y = y1; y < y2; y++) {
        u32 p = pat.p[y & 7];
        for (i32 x = 0; x < dst.wword; x++) {
            *px++ = p;
        }
    }
}

void fnt_draw_str(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, fntstr_s str, i32 mode)
{
    v2_i32   p = pos;
    texrec_s t = {0};
    t.t        = fnt.t;
    t.w        = fnt.grid_w;
    t.h        = fnt.grid_h;
    for (i32 n = 0; n < str.n; n++) {
        i32 ci = str.buf[n];
        if (32 <= ci && ci < 127) {
            t.x = ((ci - 32) % 10) * fnt.grid_w;
            t.y = ((ci - 32) / 10) * fnt.grid_h;
            gfx_spr(ctx, t, p, 0, mode);
        }
        p.x += fnt.widths[ci] + fnt.tracking;
    }
}

void fnt_draw_ascii(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, const char *text, i32 mode)
{
    fntstr_s str      = {0};
    u8       buf[256] = {0};
    str.buf           = buf;
    str.cap           = ARRLEN(buf);
    for (const char *c = text; *c != '\0'; c++) {
        if (str.n == str.cap) break;
        str.buf[str.n++] = (u8)*c;
    }
    fnt_draw_str(ctx, fnt, pos, str, mode);
}

void fnt_draw_str_mono(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, fntstr_s str, i32 mode, i32 spacing)
{
    v2_i32   p = pos;
    texrec_s t = {fnt.t, 0, 0, fnt.grid_w, fnt.grid_h};
    i32      s = spacing ? spacing : fnt.grid_w;
    for (i32 n = 0; n < str.n; n++) {
        i32 ci = str.buf[n];
        if (32 <= ci && ci < 127) {
            t.x = ((ci - 32) % 10) * fnt.grid_w;
            t.y = ((ci - 32) / 10) * fnt.grid_h;
            gfx_spr(ctx, t, p, 0, mode);
        }
        p.x += s + fnt.tracking;
    }
}

void fnt_draw_ascii_mono(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, const char *text, i32 mode, i32 spacing)
{
    fntstr_s str      = {0};
    u8       buf[256] = {0};
    str.buf           = buf;
    str.cap           = ARRLEN(buf);
    for (const char *c = text; *c != '\0'; c++) {
        if (str.n == str.cap) break;
        str.buf[str.n++] = (u8)*c;
    }
    fnt_draw_str_mono(ctx, fnt, pos, str, mode, spacing);
}

i32 fnt_length_px(fnt_s fnt, const char *txt)
{
    i32 l = 0;
    for (const char *c = txt; *c != '\0'; c++) {
        l += fnt.widths[(uint)*c];
    }
    return l;
}

i32 fnt_length_px_mono(fnt_s fnt, const char *txt, i32 spacing)
{
    i32 l = 0;
    i32 s = spacing ? spacing : fnt.grid_w;
    for (const char *c = txt; *c != '\0'; c++) {
        l += s;
    }
    return l;
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

    i32 r = (d) >> 1;
    if (d <= 2) {
        rec_i32 rf = {p.x - r, p.y - r, d, d};
        return;
    }

    i32 ymin = max_i32(ctx.clip_y1, p.y - r);
    i32 ymax = min_i32(ctx.clip_y2, p.y - r + d);
    i32 dc   = (d * d + 2) >> 2;

    for (i32 yy = ymin; yy <= ymax; yy++) {
        i32 ls = dc - pow2_i32(p.y - yy);
        if (ls < 0) continue;
        i32 xx = sqrt_u32(ls);
        i32 x1 = max_i32(p.x - xx, ctx.clip_x1);
        i32 x2 = min_i32(p.x + xx, ctx.clip_x2);
        if (x1 <= x2) {
            span_blit_s b = span_blit_gen(ctx, yy, x1, x2, mode);
            prim_blit_span(b);
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
    i32 dx = +abs_i32(b.x - a.x);
    i32 dy = -abs_i32(b.y - a.y);
    i32 sx = a.x < b.x ? +1 : -1;
    i32 sy = a.y < b.y ? +1 : -1;
    i32 er = dx + dy;
    i32 xi = a.x;
    i32 yi = a.y;

    // initialize spans
    gfx_span_s spans[512];
    i32        r    = (d) >> 1;
    i32        ymin = max_i32(min_i32(a.y, b.y) - r - 1, ctx.clip_y1);
    i32        ymax = min_i32(max_i32(a.y, b.y) + r + 1, ctx.clip_y2);
    i32        dc   = (d * d + 2) >> 2;

    for (i32 n = ymin; n <= ymax; n++) {
        gfx_span_s *s = &spans[n - ymin];
        s->x1         = U16_MAX;
        s->x2         = 0;
    }

    // generate spans
    while (1) {
        for (i32 yy = -r; yy <= d - r; yy++) {
            i32 py = yi - yy;

            if (ymin <= py && py <= ymax) {
                i32         ls = dc - pow2_i32(yy);
                i32         xx = sqrt_u32(ls + 1);
                gfx_span_s *s  = &spans[py - ymin];
                s->x1          = min_i32(s->x1, max_i32(xi - xx, ctx.clip_x1));
                s->x2          = max_i32(s->x2, min_i32(xi + xx, ctx.clip_x2));
            }
        }

        if (xi == b.x && yi == b.y) break;
        i32 e2 = er << 1;
        if (e2 >= dy) { er += dy, xi += sx; }
        if (e2 <= dx) { er += dx, yi += sy; }
    }

    // blit spans
    for (i32 n = ymin; n <= ymax; n++) {
        gfx_span_s *s = &spans[n - ymin];
        if (s->x1 <= s->x2) {
            span_blit_s b = span_blit_gen(ctx, n, s->x1, s->x2, mode);
            prim_blit_span(b);
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

void gfx_cir(gfx_ctx_s ctx, v2_i32 p, i32 r, i32 mode){
    NOT_IMPLEMENTED}

i32 cmp_int_poly(const void *a, const void *b)
{
    i32 x = *(const i32 *)a;
    i32 y = *(const i32 *)b;
    if (x < y) return -1;
    if (x > y) return +1;
    return 0;
}

void gfx_poly_fill(gfx_ctx_s ctx, v2_i32 *pt, i32 n_pt, i32 mode)
{
    i32 y1 = 0x10000;
    i32 y2 = 0;
    for (i32 i = 0; i < n_pt; i++) {
        y1 = min_i32(y1, pt[i].y);
        y2 = max_i32(y2, pt[i].y);
    }

    y1 = max_i32(y1, ctx.clip_y1);
    y2 = min_i32(y2, ctx.clip_y2);

    i32 nx[64] = {0};
    for (i32 y = y1; y <= y2; y++) {
        i32 ns = 0;
        for (i32 i = 0, j = n_pt - 1; i < n_pt; j = i, i++) {
            v2_i32 pi = pt[i];
            v2_i32 pj = pt[j];
            if (!((pi.y < y && y <= pj.y) || (pj.y < y && y <= pi.y)))
                continue;
            nx[ns++] = pi.x + ((pj.x - pi.x) * (y - pi.y)) / (pj.y - pi.y);
        }

        sort_array(nx, ns, sizeof(i32), cmp_int_poly);

        for (i32 i = 0; i < ns; i += 2) {
            i32 x1 = nx[i];
            if (ctx.clip_x2 < x1) break;
            i32 x2 = nx[i + 1];
            if (x2 <= ctx.clip_x1) continue;

            x1 = max_i32(x1, ctx.clip_x1);
            x2 = min_i32(x2, ctx.clip_x2);

            span_blit_s s = span_blit_gen(ctx, y, x1, x2, mode);
            prim_blit_span(s);
        }
    }
}

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

void gfx_spr(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode)
{
    if (!src.t.px) return;
    if (ctx.dst.fmt == TEX_FMT_OPAQUE) {
        if (flip & SPR_FLIP_X) {
            gfx_spr_sm_fx(ctx, src, pos, flip, mode);
        } else {
            gfx_spr_sm(ctx, src, pos, flip, mode);
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

    tex_s dt = ctx.dst;
    tex_s st = src.t;
    i32   nb = x2 - x1 + 1;                              // number of bits in a row
    i32   u1 = src.x - pos.x + x1;                       // first bit index in src row
    i32   v1 = src.y - pos.y + y1;                       //
    i32   od = x1 & 31;                                  // bitoffset in dst
    i32   dm = (od + nb - 1) >> 5;                       // number of touched dst words -1
    u32   ml = bswap32(0xFFFFFFFF >> od);                // mask to cut off boundary left
    u32   mr = bswap32(0xFFFFFFFF << (31 & -(od + nb))); // mask to cut off boundary right
    i32   l  = 31 & (u1 - x1);                           // word left shift amount
    i32   r  = 32 - l;                                   // word rght shift amound
    u32  *dp = &dt.px[dt.wword * y1 + ((x1 >> 5))];      // dst pixel words
    u32  *sp = &st.px[st.wword * v1 + ((u1 >> 5) << 1)]; // src pixel words

    if (dm == 0) { // combine mask left and right into one
        ml &= mr;
    }

    for (i32 y = y1; y <= y2; y++, sp += st.wword, dp += dt.wword) {
        u32 zp = bswap32(*(sp));
        u32 zm = bswap32(*(sp + 1));
        u32 p  = bswap32((u32)((u64)zp << l) | (u32)((u64)zp >> r));
        u32 m  = bswap32((u32)((u64)zm << l) | (u32)((u64)zm >> r));
        spr_blit_tile(dp + 0, p, m & ml);
        if (dm == 0) continue;
        spr_blit_tile(dp + 1, p, m & mr);
    }
}

// counter clockwise filled from a1 to a2
void gfx_fill_circle_segment(gfx_ctx_s ctx, v2_i32 p, i32 r,
                             i32 a1_q18, i32 a2_q18, i32 mode)
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