// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "gfx.h"
#include "spm.h"
#include "sys/sys.h"
#include "util/json.h"
#include "util/mathfunc.h"
#include "util/mem.h"
#include "util/str.h"

fnt_s fnt_load(const char *filename, alloc_s ma)
{
    spm_push();

    fnt_s f = {0};
    char *txt;
    if (txt_load(filename, spm_alloc, &txt) != TXT_SUCCESS) {
        sys_printf("+++ error loading font txt\n");
        return f;
    }

    f.widths = (u8 *)ma.allocf(ma.ctx, sizeof(u8) * 256);
    if (!f.widths) {
        sys_printf("+++ allocating font memory\n");
        spm_pop();
        return f;
    }

    json_s j;
    json_root(txt, &j);

    // replace .json with .tex
    char filename_tex[64];
    str_cpy(filename_tex, filename);
    char *fp = &filename_tex[str_len(filename_tex) - 5];
    assert(str_eq(fp, ".json"));
    str_cpy(fp, ".tex");

    sys_printf("  loading fnt tex: %s\n", filename_tex);
    f.t      = tex_load(filename_tex, ma);
    f.grid_w = jsonk_u32(j, "gridwidth");
    f.grid_h = jsonk_u32(j, "gridheight");

    json_key(j, "glyphwidths", &j);
    json_fchild(j, &j);
    for (int i = 0; i < 256; i++) {
        f.widths[i] = json_u32(j);
        json_next(j, &j);
    }

    spm_pop();
    return f;
}

tex_s tex_framebuffer()
{
    sys_display_s d = sys_display();
    tex_s         t = {0};
    t.fmt           = TEX_FMT_OPAQUE;
    t.px            = d.px;
    t.w             = d.w;
    t.h             = d.h;
    t.wword         = d.wword;
    return t;
}

tex_s tex_create(int w, int h, alloc_s ma)
{
    tex_s t        = {0};
    int   waligned = (w + 31) & ~31;
    int   wword    = waligned / 32;
    usize size     = sizeof(u32) * wword * h;
    void *mem      = ma.allocf(ma.ctx, size * 2); // * 2 bc of mask pixels
    if (!mem) return t;
    t.px    = (u32 *)mem;
    t.fmt   = TEX_FMT_MASK;
    t.w     = w;
    t.h     = h;
    t.wword = wword * 2;
    return t;
}

tex_s tex_load(const char *path, alloc_s ma)
{
    void *f = sys_file_open(path, SYS_FILE_R);

    uint w;
    uint h;
    sys_file_read(f, &w, sizeof(uint));
    sys_file_read(f, &h, sizeof(uint));

    usize s = ((w * h) * 2) / 8;
    tex_s t = tex_create(w, h, ma);
    sys_file_read(f, t.px, s);
    sys_file_close(f);
    return t;
}

int tex_px_at(tex_s tex, int x, int y)
{
    if (x < 0 || tex.w <= x || y < 0 || tex.h <= y) return 0;
    u32 b = bswap32(0x80000000U >> (x & 31));
    switch (tex.fmt) {
    case TEX_FMT_MASK: return ((tex.px[y * tex.wword + (x >> 5) * 2] & b) > 0);
    case TEX_FMT_OPAQUE: return ((tex.px[y * tex.wword + (x >> 5)] & b) > 0);
    }
    return 0;
}

int tex_mk_at(tex_s tex, int x, int y)
{
    if (tex.fmt == TEX_FMT_OPAQUE || x < 0 || tex.w <= x || y < 0 || tex.h <= y) return 1;
    u32 b = bswap32(0x80000000U >> (x & 31));
    return ((tex.px[y * tex.wword + (x >> 5) * 2 + 1] & b) > 0);
}

void tex_px(tex_s tex, int x, int y, int col)
{
    if (x < 0 || tex.w <= x || y < 0 || tex.h <= y) return;
    u32  b  = bswap32(0x80000000U >> (x & 31));
    int  n  = y * tex.wword + (x >> 5);
    u32 *px = NULL;
    switch (tex.fmt) {
    case TEX_FMT_MASK: px = &tex.px[y * tex.wword + (x >> 5) * 2]; break;
    case TEX_FMT_OPAQUE: px = &tex.px[y * tex.wword + (x >> 5)]; break;
    }
    *px = (col == 0 ? *px & ~b : *px | b);
}

void tex_mk(tex_s tex, int x, int y, int col)
{
    if (tex.fmt == TEX_FMT_OPAQUE || x < 0 || tex.w <= x || y < 0 || tex.h <= y) return;
    u32 *px = &tex.px[y * tex.wword + (x >> 5) * 2 + 1];
    u32  b  = bswap32(0x80000000U >> (x & 31));
    *px     = (col == 0 ? *px & ~b : *px | b);
}

void tex_outline(tex_s tex, int x, int y, int w, int h, int col, bool32 dia)
{
    spm_push();
    usize size = sizeof(u32) * tex.wword * tex.h;

    tex_s src = tex; // need to work off of a copy
    src.px    = (u32 *)spm_alloc(size);
    memcpy(src.px, tex.px, size);

    int x2 = x + w;
    int y2 = y + h;

    for (int yy = y; yy < y2; yy++) {
        for (int xx = x; xx < x2; xx++) {

            if (tex_mk_at(src, xx, yy) == 1) continue;

            for (int u = -1; u <= +1; u++) {
                for (int v = -1; v <= +1; v++) {
                    if (u == 0 && v == 0) continue;
                    if (!dia && v != 0 && u != 0) continue;

                    int t = xx + u;
                    int s = yy + v;
                    if (!(x <= t && t < x2)) continue;
                    if (!(y <= s && s < y2)) continue;

                    if (tex_mk_at(src, t, s)) {
                        tex_mk(tex, xx, yy, 1);
                        tex_px(tex, xx, yy, col);
                        goto BREAK_LOOP;
                    }
                }
            }
        BREAK_LOOP:;
        }
    }
    spm_pop();
}

gfx_ctx_s gfx_ctx_default(tex_s dst)
{
    gfx_ctx_s ctx = {0};
    ctx.dst       = dst;
    ctx.clip_x2   = dst.w - 1;
    ctx.clip_y2   = dst.h - 1;
    memset(&ctx.pat, 0xFF, sizeof(gfx_pattern_s));
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
    c.clip_x1   = max_i(x1, 0);
    c.clip_y1   = max_i(y1, 0);
    c.clip_x2   = min_i(x2, ctx.dst.w - 1);
    c.clip_y2   = min_i(y2, ctx.dst.h - 1);
    return c;
}

gfx_ctx_s gfx_ctx_clip_top(gfx_ctx_s ctx, i32 y1)
{
    gfx_ctx_s c = ctx;
    c.clip_y1   = max_i(y1, 0);
    return c;
}

gfx_ctx_s gfx_ctx_clip_bot(gfx_ctx_s ctx, i32 y2)
{
    gfx_ctx_s c = ctx;
    c.clip_y2   = min_i(y2, ctx.dst.h - 1);
    return c;
}

gfx_ctx_s gfx_ctx_clip_left(gfx_ctx_s ctx, i32 x1)
{
    gfx_ctx_s c = ctx;
    c.clip_x1   = max_i(x1, 0);
    return c;
}

gfx_ctx_s gfx_ctx_clip_right(gfx_ctx_s ctx, i32 x2)
{
    gfx_ctx_s c = ctx;
    c.clip_x2   = min_i(x2, ctx.dst.w - 1);
    return c;
}

gfx_ctx_s gfx_ctx_clipr(gfx_ctx_s ctx, rec_i32 r)
{
    return gfx_ctx_clip(ctx, r.x, r.y, r.x + r.w - 1, r.x + r.h - 1);
}

gfx_ctx_s gfx_ctx_clipwh(gfx_ctx_s ctx, i32 x, i32 y, i32 w, i32 h)
{
    return gfx_ctx_clip(ctx, x, y, x + w - 1, x + h - 1);
}

gfx_pattern_s gfx_pattern_4x4(int p0, int p1, int p2, int p3)
{
    gfx_pattern_s pat  = {0};
    int           p[4] = {p0, p1, p2, p3};
    for (int i = 0; i < 4; i++) {
        u32 pa       = ((u32)p[i] << 4) | ((u32)p[i]);
        u32 pb       = (pa << 24) | (pa << 16) | (pa << 8) | (pa);
        pat.p[i + 0] = pb;
        pat.p[i + 4] = pb;
    }
    return pat;
}

gfx_pattern_s gfx_pattern_8x8(int p0, int p1, int p2, int p3,
                              int p4, int p5, int p6, int p7)
{
    gfx_pattern_s pat  = {0};
    int           p[8] = {p0, p1, p2, p3, p4, p5, p6, p7};
    for (int i = 0; i < 8; i++) {
        u32 pp   = (u32)p[i];
        pat.p[i] = (pp << 24) | (pp << 16) | (pp << 8) | (pp);
    }
    return pat;
}

gfx_pattern_s gfx_pattern_bayer_4x4(int i)
{
    static const u32 ditherpat[17 * 4] = {
        0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
        0x88888888U, 0x00000000U, 0x00000000U, 0x00000000U,
        0x88888888U, 0x00000000U, 0x22222222U, 0x00000000U,
        0xAAAAAAAAU, 0x00000000U, 0x22222222U, 0x00000000U,
        0xAAAAAAAAU, 0x00000000U, 0xAAAAAAAAU, 0x00000000U,
        0xAAAAAAAAU, 0x44444444U, 0xAAAAAAAAU, 0x00000000U,
        0xAAAAAAAAU, 0x44444444U, 0xAAAAAAAAU, 0x11111111U,
        0xAAAAAAAAU, 0x55555555U, 0xAAAAAAAAU, 0x11111111U,
        0xAAAAAAAAU, 0x55555555U, 0xAAAAAAAAU, 0x55555555U,
        0xEEEEEEEEU, 0x55555555U, 0xAAAAAAAAU, 0x55555555U,
        0xEEEEEEEEU, 0x55555555U, 0xBBBBBBBBU, 0x55555555U,
        0xFFFFFFFFU, 0x55555555U, 0xBBBBBBBBU, 0x55555555U,
        0xFFFFFFFFU, 0x55555555U, 0xFFFFFFFFU, 0x55555555U,
        0xFFFFFFFFU, 0xDDDDDDDDU, 0xFFFFFFFFU, 0x55555555U,
        0xFFFFFFFFU, 0xDDDDDDDDU, 0xFFFFFFFFU, 0x77777777U,
        0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x77777777U,
        0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU};

    const u32    *p   = &ditherpat[clamp_i(i, 0, 16) << 2];
    gfx_pattern_s pat = {{p[0], p[1], p[2], p[3], p[0], p[1], p[2], p[3]}};
    return pat;
}

gfx_pattern_s gfx_pattern_hor_stripes_4x4(int i)
{
    static const u32 ditherpat[5 * 8] = {
        0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
        0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
        0xFFFFFFFFU, 0x00000000U, 0x00000000U, 0x00000000U,
        0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
        0xFFFFFFFFU, 0x00000000U, 0x00000000U, 0x00000000U,
        0xFFFFFFFFU, 0x00000000U, 0x00000000U, 0x00000000U,
        0xFFFFFFFFU, 0x00000000U, 0xFFFFFFFFU, 0x00000000U,
        0xFFFFFFFFU, 0x00000000U, 0xFFFFFFFFU, 0x00000000U,
        0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
        0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU};

    const u32    *p   = &ditherpat[clamp_i(i, 0, 4) << 3];
    gfx_pattern_s pat = {{p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]}};
    return pat;
}

gfx_pattern_s gfx_pattern_interpolate(int num, int den)
{
    return gfx_pattern_bayer_4x4((num * 16 + (den / 2)) / den);
}

gfx_pattern_s gfx_pattern_interpolate_hor_stripes(int num, int den)
{
    return gfx_pattern_hor_stripes_4x4((num * 5 + (den / 2)) / den);
}

void tex_clr(tex_s dst, int col)
{
    int N = dst.wword * dst.h;
    switch (col) {
    case TEX_CLR_BLACK:
        for (int n = 0; n < N; n++) {
            dst.px[n] = 0xFFFFFFFFU;
        }
        break;
    case TEX_CLR_WHITE:
        switch (dst.fmt) {
        case TEX_FMT_OPAQUE:
            for (int n = 0; n < N; n++) {
                dst.px[n] = 0U;
            }
            break;
        case TEX_FMT_MASK:
            for (int n = 0; n < N; n += 2) {
                dst.px[n + 0] = 0U;
                dst.px[n + 1] = 0xFFFFFFFFU;
            }
            break;
        }
        break;
    case TEX_CLR_TRANSPARENT:
        if (dst.fmt == TEX_FMT_OPAQUE) break;
        for (int n = 0; n < N; n++) {
            dst.px[n] = 0U;
        }
        break;
    }
}

// used for simple sprites
typedef struct {
    int           dmax; // count of dst words -1
    u32           ml;   // boundary mask left
    u32           mr;   // boundary mask right
    int           mode; // drawing mode
    int           doff; // bitoffset of first dst bit
    u32          *dp;   // pixel
    int           y;
    gfx_pattern_s pat;
    int           dadd;
} span_blit_s;

static void apply_spr_mode(u32 *restrict dp, u32 *restrict dm, u32 sp, u32 sm, int mode)
{
    switch (mode) {
    case SPR_MODE_INV: sp = ~sp; // fallthrough
    case SPR_MODE_COPY: *dp = (*dp & ~sm) | (sp & sm); break;
    case SPR_MODE_XOR: sp = ~sp; // fallthrough
    case SPR_MODE_NXOR: *dp = (*dp & ~sm) | ((*dp ^ sp) & sm); break;
    case SPR_MODE_WHITE_ONLY: sm &= sp; // fallthrough
    case SPR_MODE_WHITE: *dp |= sm; break;
    case SPR_MODE_BLACK_ONLY: sm &= ~sp; // fallthrough
    case SPR_MODE_BLACK: *dp &= ~sm; break;
    }

    if (dm) *dm |= sm;
}

span_blit_s span_blit_gen(gfx_ctx_s ctx, int y, int x1, int x2, int mode)
{
    int nbit = (x2 + 1) - x1; // number of bits in a row to blit
    int dsti = (x1 >> 5) + y * ctx.dst.wword;

    span_blit_s info = {0};
    info.y           = y;
    info.doff        = x1 & 31;
    info.dmax        = (info.doff + nbit - 1) >> 5;                        // number of touched dst words -1
    info.mode        = mode;                                               // sprite masking mode
    info.ml          = bswap32(0xFFFFFFFFU >> (31 & info.doff));           // mask to cut off boundary left
    info.mr          = bswap32(0xFFFFFFFFU << (31 & (-info.doff - nbit))); // mask to cut off boundary right
    info.dp          = &ctx.dst.px[dsti];
    info.dadd        = ctx.dst.fmt == TEX_FMT_MASK ? 2 : 1;
    info.pat         = ctx.pat;
    return info;
}

void gfx_spr_rotscl(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, v2_i32 origin, f32 angle,
                    f32 sclx, f32 scly)
{
    tex_s dst = ctx.dst;
    origin.x += src.r.x;
    origin.y += src.r.y;
    m33_f32 m1 = m33_offset(+(f32)origin.x, +(f32)origin.y);
    m33_f32 m3 = m33_offset(-(f32)pos.x - (f32)origin.x, -(f32)pos.y - (f32)origin.y);
    m33_f32 m4 = m33_rotate(angle);
    m33_f32 m5 = m33_scale(sclx, scly);

    m33_f32 m = m33_identity();
    m         = m33_mul(m, m3);
    m         = m33_mul(m, m5);
    m         = m33_mul(m, m4);
    m         = m33_mul(m, m1);

    int x1 = 0;
    int y1 = 0;
    int x2 = ctx.dst.w - 1;
    int y2 = ctx.dst.h - 1;
    int w1 = x1 >> 5;
    int w2 = x2 >> 5;

    f32 u1 = (f32)(src.r.x);
    f32 v1 = (f32)(src.r.y);
    f32 u2 = (f32)(src.r.x + src.r.w);
    f32 v2 = (f32)(src.r.y + src.r.h);

    for (int y = y1; y <= y2; y++) {
        u32 pat = ctx.pat.p[y & 7];
        for (int wi = w1; wi <= w2; wi++) {

            int p1 = wi == w1 ? x1 & 31 : 0;
            int p2 = wi == w2 ? x2 & 31 : 31;
            u32 sp = 0;
            u32 sm = 0;

            for (int p = p1; p <= p2; p++) {
                int x = wi * 32 + p;

                f32 t = (f32)(x + src.r.x);
                f32 s = (f32)(y + src.r.y);
                int u = (int)(t * m.m[0] + s * m.m[1] + m.m[2] + .5f);
                int v = (int)(t * m.m[3] + s * m.m[4] + m.m[5] + .5f);
                if (!(u1 <= u && u < u2)) continue;
                if (!(v1 <= v && v < v2)) continue;
                if (tex_px_at(src.t, u, v) != 0) sp |= 0x80000000U >> p;
                if (tex_mk_at(src.t, u, v) != 0) sm |= 0x80000000U >> p;
            }

            u32 *dp = NULL;
            u32 *dm = NULL;
            switch (ctx.dst.fmt) {
            case TEX_FMT_OPAQUE: dp = &ctx.dst.px[y * dst.wword + wi]; break;
            case TEX_FMT_MASK:
                dp = &dst.px[y * dst.wword + wi * 2];
                dm = &dst.px[y * dst.wword + wi * 2 + 1];
                break;
            }

            apply_spr_mode(dp, dm, bswap32(sp), bswap32(sm) & pat, 0);
        }
    }
}

void gfx_spr_rotated(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, v2_i32 origin, f32 angle)
{
    gfx_spr_rotscl(ctx, src, pos, origin, angle, 1.f, 1.f);
}

static void apply_prim_mode(u32 *restrict dp, u32 *restrict dm, u32 sm, int mode, u32 pt)
{
    switch (mode) {
    case PRIM_MODE_INV: sm &= pt, *dp = (*dp & ~sm) | (~*dp & sm); break;
    case PRIM_MODE_WHITE: sm &= pt, *dp |= sm; break;
    case PRIM_MODE_BLACK: sm &= pt, *dp &= ~sm; break;
    case PRIM_MODE_WHITE_BLACK: pt = ~pt; // fallthrough
    case PRIM_MODE_BLACK_WHITE: *dp = (*dp & ~(sm & pt)) | (sm & ~pt); break;
    }

    if (dm) *dm |= sm;
}

static void prim_blit_span(span_blit_s info)
{
    u32 *restrict dp = (u32 *restrict)info.dp;
    u32 pt           = info.pat.p[info.y & 7];
    u32 m            = info.ml;
    for (int i = 0; i < info.dmax; i++) {
        apply_prim_mode(dp, info.dadd == 2 ? dp + 1 : NULL, m, info.mode, pt);
        m = 0xFFFFFFFFU;
        dp += info.dadd;
    }
    apply_prim_mode(dp, info.dadd == 2 ? dp + 1 : NULL, m & info.mr, info.mode, pt);
}

void gfx_rec_fill(gfx_ctx_s ctx, rec_i32 rec, int mode)
{
    tex_s dtex = ctx.dst; // area bounds on canvas [x1/y1, x2/y2]
    int   x1   = max_i(rec.x, ctx.clip_x1);
    int   y1   = max_i(rec.y, ctx.clip_y1);
    int   x2   = min_i(rec.x + rec.w - 1, ctx.clip_x2);
    int   y2   = min_i(rec.y + rec.h - 1, ctx.clip_y2);
    if (x2 < x1) return;

    span_blit_s info = span_blit_gen(ctx, y1, x1, x2, mode);
    for (info.y = y1; info.y <= y2; info.y++) {
        prim_blit_span(info);

        info.dp += dtex.wword;
    }
}

void fnt_draw_str(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, fntstr_s str, int mode)
{
    v2_i32   p = pos;
    texrec_s t;
    t.t   = fnt.t;
    t.r.w = fnt.grid_w;
    t.r.h = fnt.grid_h;
    for (int n = 0; n < str.n; n++) {
        int ci = str.buf[n];
        t.r.x  = (ci & 31) * fnt.grid_w;
        t.r.y  = (ci >> 5) * fnt.grid_h;
        gfx_spr(ctx, t, p, 0, mode);
        p.x += fnt.widths[ci];
    }
}

void fnt_draw_ascii(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, const char *text, int mode)
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

int fnt_length_px(fnt_s fnt, const char *txt)
{
    int l = 0;
    for (const char *c = txt; *c != '\0'; c++) {
        l += fnt.widths[(uint)*c];
    }
    return l;
}

void gfx_tri_fill(gfx_ctx_s ctx, tri_i32 t, int mode)
{
    v2_i32 t0 = t.p[0];
    v2_i32 t1 = t.p[1];
    v2_i32 t2 = t.p[2];
    if (t0.y > t1.y) SWAP(v2_i32, t0, t1);
    if (t0.y > t2.y) SWAP(v2_i32, t0, t2);
    if (t1.y > t2.y) SWAP(v2_i32, t1, t2);
    int th = t2.y - t0.y;
    if (th == 0) return;
    int h1 = t1.y - t0.y + 1;
    int h2 = t2.y - t1.y + 1;
    i32 d0 = t2.x - t0.x;
    i32 d1 = t1.x - t0.x;
    i32 d2 = t2.x - t1.x;
    i32 y0 = max_i(ctx.clip_y1, t0.y);
    i32 y1 = min_i(ctx.clip_y2, t1.y);
    i32 y2 = min_i(ctx.clip_y2, t2.y);
    for (int y = y0; y <= y1; y++) {
        i32 x1 = t0.x + (d0 * (y - t0.y)) / th;
        i32 x2 = t0.x + (d1 * (y - t0.y)) / h1;
        if (x2 < x1) SWAP(i32, x1, x2);
        span_blit_s info = span_blit_gen(ctx, y, x1, x2, mode);
        prim_blit_span(info);
    }

    for (int y = y1; y <= y2; y++) {
        i32 x1 = t0.x + (d0 * (y - t0.y)) / th;
        i32 x2 = t1.x + (d2 * (y - t1.y)) / h2;
        if (x2 < x1) SWAP(i32, x1, x2);
        span_blit_s info = span_blit_gen(ctx, y, x1, x2, mode);
        prim_blit_span(info);
    }
}

void gfx_cir_fill(gfx_ctx_s ctx, v2_i32 p, int d, int mode)
{
    int y1 = max_i(p.y - (d >> 1), ctx.clip_y1);
    int y2 = min_i(y1 + d, ctx.clip_y2);
    int r2 = d * d + 1; // radius doubled^2
    for (int y = y1; y <= y2; y++) {
        int yy = (y - p.y) << 1;
        int xx = sqrt_i32(r2 - yy * yy) >> 1;
        int x1 = max_i(p.x - xx, ctx.clip_x1);
        int x2 = min_i(p.x + xx, ctx.clip_x2);
        if (x2 < x1) continue;
        span_blit_s info = span_blit_gen(ctx, y, x1, x2, mode);
        prim_blit_span(info);
    }
}

void gfx_lin(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, int mode)
{
    gfx_lin_thick(ctx, a, b, mode, 1);
}

void gfx_lin_thick(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, int mode, int r)
{
#if 1
    int    dx = +abs_i(b.x - a.x);
    int    dy = -abs_i(b.y - a.y);
    int    sx = a.x < b.x ? +1 : -1;
    int    sy = a.y < b.y ? +1 : -1;
    int    er = dx + dy;
    v2_i32 pi = a;

    while (1) {
        gfx_cir_fill(ctx, pi, r, mode);

        if (pi.x == b.x && pi.y == b.y) break;
        int e2 = er << 1;
        if (e2 >= dy) { er += dy, pi.x += sx; }
        if (e2 <= dx) { er += dx, pi.y += sy; }
    }

#else
    line_i32 l0 = {a, b};

    v2_i32 ldt = v2_sub(a, b);

    if (ldt.y == 0) {
        gfx_lin(ctx, a, b, mode);
        return;
    }

    ldt        = v2_shl(ldt, 8);
    v2_i32 odt = {ldt.y, -ldt.x};
    odt        = v2_setlen(odt, r << 8);

    v2_i32 c = v2_add(v2_shl(a, 8), odt);
    v2_i32 d = v2_add(v2_shl(b, 8), odt);

    v2_i32 e = v2_sub(v2_shl(a, 8), odt);
    v2_i32 f = v2_sub(v2_shl(b, 8), odt);

    c = v2_shr(c, 8);
    d = v2_shr(d, 8);
    e = v2_shr(e, 8);
    f = v2_shr(f, 8);

    tri_i32 t1 = {c, d, e};
    tri_i32 t2 = {c, e, f};
    gfx_tri_fill(ctx, t1, mode);
    gfx_tri_fill(ctx, t2, mode);

#endif
}

void gfx_rec(gfx_ctx_s ctx, rec_i32 r, int mode)
{
    NOT_IMPLEMENTED
}

void gfx_tri(gfx_ctx_s ctx, tri_i32 t, int mode)
{
    gfx_lin_thick(ctx, t.p[0], t.p[1], mode, 2);
    gfx_lin_thick(ctx, t.p[0], t.p[2], mode, 2);
    gfx_lin_thick(ctx, t.p[1], t.p[2], mode, 2);
}

void gfx_cir(gfx_ctx_s ctx, v2_i32 p, int r, int mode)
{
    NOT_IMPLEMENTED
}