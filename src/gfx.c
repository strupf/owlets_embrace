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

fnt_s fnt_load(const char *filename, void *(*allocf)(usize s))
{
    spm_push();

    fnt_s f = {0};
    char *txt;
    if (txt_load(filename, spm_alloc, &txt) != TXT_SUCCESS) {
        sys_printf("+++ error loading font txt\n");
        return f;
    }

    f.widths = (u8 *)allocf(sizeof(u8) * 256);
    if (!f.widths) {
        sys_printf("+++ allocating font memory\n");
        spm_pop();
        return f;
    }

    json_s j;
    json_root(txt, &j);

    // create filename of the texture
    // font_example.json     <- name of font config
    // font_example_tex.json <- name of the texture
    char filename_tex[64];
    str_cpy(filename_tex, filename);
    char *fp = &filename_tex[str_len(filename_tex) - 5];
    assert(*fp == '.');
    str_cpy(fp, ".tex");

    f.t      = tex_load(filename_tex, allocf);
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
    t.px            = d.px;
    t.w             = d.w;
    t.h             = d.h;
    t.wbyte         = d.wbyte;
    t.wword         = d.wword;
    return t;
}

tex_s tex_create(int w, int h, void *(*allocf)(usize s))
{
    tex_s t        = {0};
    int   waligned = (w + 31) & ~31;
    int   wword    = waligned / 32;
    int   wbyte    = waligned / 8;
    usize size     = sizeof(u8) * wbyte * h;
    void *mem      = allocf(size * 2); // * 2 bc of mask pixels
    if (!mem) return t;
    t.px    = (u8 *)mem;
    t.mk    = (u8 *)mem + size;
    t.w     = w;
    t.h     = h;
    t.wbyte = wbyte;
    t.wword = wword;
    return t;
}

tex_s tex_load(const char *path, void *(*allocf)(usize s))
{
    void *f = sys_file_open(path, SYS_FILE_R);

    int w;
    int h;
    sys_file_read(f, &w, sizeof(int));
    sys_file_read(f, &h, sizeof(int));

    usize s = ((w * h) * 2) / 8;
    tex_s t = tex_create(w, h, allocf);
    sys_file_read(f, t.px, s);
    sys_file_close(f);
    return t;
}

gfx_ctx_s gfx_ctx_default(tex_s dst)
{
    gfx_ctx_s ctx = {0};
    ctx.dst       = dst;
    memset(&ctx.pat, 0xFF, sizeof(gfx_pattern_s));
    return ctx;
}

gfx_pattern_s gfx_pattern_4x4(int p0, int p1, int p2, int p3)
{
    gfx_pattern_s pat;
    int           p[4] = {p0, p1, p2, p3};
    for (int i = 0; i < 4; i++) {
        u32 pp       = ((u32)p[i] << 4) | ((u32)p[i]);
        pp           = (pp << 24) | (pp << 16) | (pp << 8) | (pp);
        pat.p[i + 0] = pp;
        pat.p[i + 4] = pp;
    }
    return pat;
}

gfx_pattern_s gfx_pattern_8x8(int p0, int p1, int p2, int p3,
                              int p4, int p5, int p6, int p7)
{
    gfx_pattern_s pat;
    int           p[8] = {p0, p1, p2, p3, p4, p5, p6, p7};
    for (int i = 0; i < 8; i++) {
        u32 pp   = (u32)p[i];
        pat.p[i] = (pp << 24) | (pp << 16) | (pp << 8) | (pp);
    }
    return pat;
}

gfx_pattern_s gfx_pattern_interpolate(int num, int den)
{
    return gfx_pattern_bayer_4x4((num * 16 + (den / 2)) / den);
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

    u32          *p   = &ditherpat[clamp_i(i, 0, 16) << 2];
    gfx_pattern_s pat = {p[0], p[1], p[2], p[3], p[0], p[1], p[2], p[3]};
    return pat;
}

void tex_clr(tex_s dst, int col)
{
    usize s = (usize)(dst.wbyte * dst.h);
    switch (col) {
    case TEX_CLR_BLACK:
        memset(dst.px, 0, s);
        if (dst.mk) memset(dst.mk, 0xFF, s);
        break;
    case TEX_CLR_WHITE:
        memset(dst.px, 0xFF, s * (dst.mk ? 2 : 1));
        break;
    case TEX_CLR_TRANSPARENT:
        if (dst.mk) memset(dst.px, 0, s * 2);
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
    u32          *dp;
    u32          *dm;
    int           y;
    gfx_pattern_s pat;
    //
    // sprite only
    int           shift; // amount of bitshift needed to align
    int           smax;  // count of src words -1
    int           soff;  // bitoffset of first src bit
    u32          *sp;
    u32          *sm;
} span_blit_s;

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
    info.dp          = &((u32 *)ctx.dst.px)[dsti];
    info.dm          = ctx.dst.mk ? &((u32 *)ctx.dst.mk)[dsti] : NULL;
    info.pat         = ctx.pat;
    return info;
}

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

static void spr_blit_row_rev(span_blit_s info)
{
    u32 pt                 = info.pat.p[info.y & 7];
    u32 *restrict dp       = (u32 *restrict)(info.dp);
    u32 *restrict dm       = (u32 *restrict)(info.dm);
    const u32 *restrict sp = (u32 *restrict)(info.sp + info.smax);
    const u32 *restrict sm = (u32 *restrict)(info.sm ? info.sm + info.smax : NULL);
    int a                  = info.shift;
    int b                  = 32 - a;

    if (a == 0) { // same alignment, fast path
        u32 p = brev32(*sp--);
        u32 m = sm ? brev32(*sm--) & info.ml : info.ml;
        for (int i = 0; i < info.dmax; i++) {
            apply_spr_mode(dp, dm, p, m & pt, info.mode);
            p = brev32(*sp--);
            m = sm ? brev32(*sm--) : 0xFFFFFFFFU;
            dp++;
            if (dm) dm++;
        }
        apply_spr_mode(dp, dm, p, m & (pt & info.mr), info.mode);
        return;
    }

    { // first word
        u32 p = 0, m = 0;
        if (info.soff > info.doff) {
            p = brev32(bswap32(*sp)) << a;
            m = sm ? brev32(bswap32(*sm)) << a : 0xFFFFFFFFU;
            if (info.smax > 0) {
                sp--;
                if (sm) sm--;
            }
        }

        p = bswap32(p | (brev32(bswap32(*sp)) >> b));
        m = sm ? bswap32(m | (brev32(bswap32(*sm)) >> b)) & info.ml : info.ml;

        if (info.dmax == 0) {
            apply_spr_mode(dp, dm, p, m & (pt & info.mr), info.mode);
            return; // only one word long
        }

        apply_spr_mode(dp, dm, p, m & pt, info.mode);
        dp++;
        if (dm) dm++;
    }

    // middle words without first and last word
    for (int i = 1; i < info.dmax; i++) {
        u32 p = bswap32((brev32(bswap32(*sp)) << a) | (brev32(bswap32(*(sp - 1))) >> b));
        u32 m = sm ? bswap32((brev32(bswap32(*sm)) << a) | (brev32(bswap32(*(sm - 1))) >> b)) : 0xFFFFFFFFU;
        apply_spr_mode(dp, dm, p, m & pt, info.mode);
        sp--;
        dp++;
        if (sm) sm--;
        if (dm) dm++;
    }

    { // last word
        u32 p = brev32(bswap32(*sp)) << a;
        u32 m = sm ? brev32(bswap32(*sm)) << a : 0xFFFFFFFFU;
        if (sp > info.sp) {
            p |= brev32(bswap32(*--sp)) >> b;
            if (sm) m |= brev32(bswap32(*--sm)) >> b;
        }
        p = bswap32(p);
        m = bswap32(m);
        apply_spr_mode(dp, dm, p, m & (pt & info.mr), info.mode);
    }
}

static void spr_blit_row(span_blit_s info)
{
    u32 pt                 = info.pat.p[info.y & 7];
    u32 *restrict dp       = (u32 *restrict)(info.dp);
    u32 *restrict dm       = (u32 *restrict)(info.dm);
    const u32 *restrict sp = (u32 *restrict)(info.sp);
    const u32 *restrict sm = (u32 *restrict)(info.sm);
    int a                  = info.shift;
    int b                  = 32 - a;

    if (a == 0) { // same alignment, fast path
        u32 p = *sp++;
        u32 m = sm ? *sm++ & info.ml : info.ml;
        for (int i = 0; i < info.dmax; i++) {
            apply_spr_mode(dp, dm, p, m & pt, info.mode);
            p = *sp++;
            m = sm ? *sm++ : 0xFFFFFFFFU;
            dp++;
            if (dm) dm++;
        }
        apply_spr_mode(dp, dm, p, m & (pt & info.mr), info.mode);
        return;
    }

    { // first word
        u32 p = 0, m = 0;
        if (info.soff > info.doff) {
            p = bswap32(*sp) << a;
            m = sm ? bswap32(*sm) << a : 0xFFFFFFFFU;
            if (info.smax > 0) {
                sp++;
                if (sm) sm++;
            }
        }

        p = bswap32(p | (bswap32(*sp) >> b));
        m = sm ? bswap32(m | (bswap32(*sm) >> b)) & info.ml : info.ml;
        if (info.dmax == 0) {
            apply_spr_mode(dp, dm, p, m & (pt & info.mr), info.mode);
            return; // only one word long
        }

        apply_spr_mode(dp, dm, p, m & pt, info.mode);
        dp++;
        if (dm) dm++;
    }

    // middle words without first and last word
    for (int i = 1; i < info.dmax; i++) {
        u32 p = bswap32((bswap32(*sp) << a) | (bswap32(*(sp + 1)) >> b));
        u32 m = sm ? bswap32((bswap32(*sm) << a) | (bswap32(*(sm + 1)) >> b)) & pt : pt;
        apply_spr_mode(dp, dm, p, m, info.mode);
        sp++;
        dp++;
        if (sm) sm++;
        if (dm) dm++;
    }

    { // last word
        u32 p = bswap32(*sp) << a;
        u32 m = sm ? bswap32(*sm) << a : 0xFFFFFFFFU;
        if (sp < info.sp + info.smax) { // this is different for reversed blitting!
            p |= bswap32(*++sp) >> b;
            if (sm) m |= bswap32(*++sm) >> b;
        }
        p = bswap32(p);
        m = bswap32(m);
        apply_spr_mode(dp, dm, p, m & (pt & info.mr), info.mode);
    }
}

void gfx_spr(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, int flip, int mode)
{
    tex_s stex = src.t;
    tex_s dtex = ctx.dst;                            // area bounds on canvas [x1/y1, x2/y2)
    int   x1   = max_i(pos.x, 0);                    // inclusive
    int   y1   = max_i(pos.y, 0);                    // inclusive
    int   x2   = min_i(pos.x + src.r.w, dtex.w) - 1; // inclusive
    int   y2   = min_i(pos.y + src.r.h, dtex.h) - 1; // inclusive
    if (x1 > x2) return;
    span_blit_s info = span_blit_gen(ctx, y1, x1, x2, mode);

    int nbit = (x2 + 1) - x1; // number of bits in a row to blit
    int srci;                 // first word index in src
    int u1;                   // first bit index in src row

    if (flip & SPR_FLIP_X) { // flipping needs special care
        u1        = src.r.x + pos.x + src.r.w - (x2 + 1);
        info.soff = (-u1 - nbit) & 31;
    } else {
        u1        = src.r.x - pos.x + x1; // first bit index in src row
        info.soff = u1 & 31;
    }

    if (flip & SPR_FLIP_Y) {
        srci = (u1 >> 5) + stex.wword * (src.r.y + pos.y - y1 + src.r.h - 1);
    } else {
        srci = (u1 >> 5) + stex.wword * (src.r.y - pos.y + y1);
    }

    info.sp    = &((u32 *restrict)stex.px)[srci];                  // src black/white
    info.sm    = stex.mk ? &((u32 *restrict)stex.mk)[srci] : NULL; // src opaque/transparent
    info.shift = (info.soff - info.doff) & 31;                     // word shift amount
    info.smax  = (info.soff + nbit - 1) >> 5;                      // number of touched src words -1

    for (info.y = y1; info.y <= y2; info.y++) {
        if (flip & SPR_FLIP_X) {
            spr_blit_row_rev(info);
        } else {
            spr_blit_row(info);
        }

        if (flip & SPR_FLIP_Y) {
            info.sp -= stex.wword;
            if (info.sm) info.sm -= stex.wword;
        } else {
            info.sp += stex.wword;
            if (info.sm) info.sm += stex.wword;
        }

        info.dp += dtex.wword;
        if (info.dm) info.dm += dtex.wword;
    }
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

static void prim_blit_row(span_blit_s info)
{
    u32 *restrict dp = (u32 *restrict)info.dp;
    u32 *restrict dm = (u32 *restrict)info.dm;

    u32 pt = info.pat.p[info.y & 7];
    u32 m  = info.ml;
    for (int i = 0; i < info.dmax; i++) {
        apply_prim_mode(dp, dm, m, info.mode, pt);
        m = 0xFFFFFFFFU;
        dp++;
        if (dm) dm++;
    }
    apply_prim_mode(dp, dm, m & info.mr, info.mode, pt);
}

void gfx_rec_fill(gfx_ctx_s ctx, rec_i32 rec, int mode)
{
    tex_s dtex = ctx.dst; // area bounds on canvas [x1/y1, x2/y2]
    int   x1   = max_i(rec.x, 0);
    int   y1   = max_i(rec.y, 0);
    int   x2   = min_i(rec.x + rec.w, dtex.w) - 1;
    int   y2   = min_i(rec.y + rec.h, dtex.h) - 1;
    if (x1 > x2) return;

    span_blit_s info = span_blit_gen(ctx, y1, x1, x2, mode);
    for (info.y = y1; info.y <= y2; info.y++) {
        prim_blit_row(info);

        info.dp += dtex.wword;
        if (info.dm) info.dm += dtex.wword;
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
    spm_push();
    fntstr_s str = {0};
    str.buf      = (u8 *)spm_alloc(sizeof(u8) * 1024);
    str.cap      = 1024;
    for (const char *c = text; *c != '\0'; c++) {
        if (str.n == str.cap) break;
        str.buf[str.n++] = (u8)*c;
    }
    fnt_draw_str(ctx, fnt, pos, str, mode);
    spm_pop();
}

void gfx_tri_fill(gfx_ctx_s ctx, tri_i32 t, int mode)
{
    NOT_IMPLEMENTED
}

void gfx_cir_fill(gfx_ctx_s ctx, v2_i32 p, int r, int mode)
{
    tex_s dtex = ctx.dst;
    int   y1   = max_i(p.y - r, 0);          // inclusive
    int   y2   = min_i(p.y + r, dtex.h) - 1; // inclusive
    for (int y = y1; y <= y2; y++) {
        int xx = r;
        int yy = y - p.y;
        while (xx >= 0) {
            if ((xx * xx + yy * yy) - (r * r) <= 0) break;
            xx--;
        }
        int x1 = max_i(p.x - xx, 0);
        int x2 = min_i(p.x + xx, dtex.w) - 1;
        if (x1 > x2) continue;
        span_blit_s info = span_blit_gen(ctx, y, x1, x2, mode);
        prim_blit_row(info);
    }
}

void gfx_lin(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, int mode)
{
}

void gfx_lin_thick(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, int mode, int r)
{
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
}

void gfx_rec(gfx_ctx_s ctx, rec_i32 r, int mode)
{
    NOT_IMPLEMENTED
}

void gfx_tri(gfx_ctx_s ctx, tri_i32 t, int mode)
{
    NOT_IMPLEMENTED
}

void gfx_cir(gfx_ctx_s ctx, v2_i32 p, int r, int mode)
{
    NOT_IMPLEMENTED
}