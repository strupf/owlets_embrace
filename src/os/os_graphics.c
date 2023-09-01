// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os_internal.h"

tex_s tex_create(int w, int h, bool32 mask)
{
        // bytes per row - rows aligned to 32 bit
        int    w_word = ((w - 1) / 32) + 1;
        int    w_byte = w_word * 4;
        size_t s      = sizeof(u8) * w_byte * h;

        // twice the size (1 bit white/black, 1 bit transparent/opaque)
        void *mem = assetmem_alloc(s * (mask ? 2 : 1));
        u8   *px  = (u8 *)mem;
        u8   *mk  = (u8 *)(mask ? px + s : NULL);
        tex_s t   = {px, mk, w_word, w_byte, w, h};
        return t;
}

tex_s tex_load(const char *filename)
{
        os_spmem_push();
        char *txtbuf = txt_read_file_alloc(filename, os_spmem_alloc);
        jsn_s j;
        jsn_root(txtbuf, &j);
        i32   w = jsn_intk(j, "width");
        i32   h = jsn_intk(j, "height");
        char *c = jsn_strkptr(j, "data");
        tex_s t = tex_create(w, h, 1);
        for (int y = 0; y < h * 2; y++) {
                for (int x = 0; x < t.w_byte; x++) {
                        int c1 = (char_hex_to_int(*c++)) << 4;
                        int c2 = (char_hex_to_int(*c++));

                        t.px[x + y * t.w_byte] = (c1 | c2);
                }
        }
        os_spmem_pop();
        return t;
}

tex_s tex_put_load(int ID, const char *filename)
{
        tex_s t = tex_load(filename);
        tex_put(ID, t);
        return t;
}

tex_s tex_put(int ID, tex_s t)
{
        ASSERT(0 <= ID && ID < NUM_TEXID);
        g_os.tex_tab[ID] = t;
        return t;
}

tex_s tex_get(int ID)
{
        ASSERT(0 <= ID && ID < NUM_TEXID);
        return g_os.tex_tab[ID];
}

fntstr_s fntstr_create(int numchars, void *(*allocfunc)(size_t))
{
        fntstr_s str = {0};
        str.chars    = (fntchar_s *)allocfunc(sizeof(fntchar_s) * numchars);
        str.c        = numchars;
        return str;
}

fnt_s fnt_put_load(int ID, const char *filename)
{
        fnt_s f = fnt_load(filename);
        fnt_put(ID, f);
        return f;
}

void fnt_put(int ID, fnt_s f)
{
        ASSERT(0 <= ID && ID < NUM_FNTID);
        g_os.fnt_tab[ID] = f;
}

fnt_s fnt_get(int ID)
{
        ASSERT(0 <= ID && ID < NUM_FNTID);
        return g_os.fnt_tab[ID];
}

fnt_s fnt_load(const char *filename)
{
        os_spmem_push();
        char *txtbuf = txt_read_file_alloc(filename, os_spmem_alloc);
        jsn_s j;
        jsn_root(txtbuf, &j);
        fnt_s fnt      = {0};
        fnt.tex        = tex_get(jsn_intk(j, "texID"));
        fnt.gridw      = jsn_intk(j, "gridwidth");
        fnt.gridh      = jsn_intk(j, "gridheight");
        fnt.lineheight = jsn_intk(j, "lineheight");
        int n          = 0;
        foreach_jsn_childk (j, "glyphwidths", jt) {
                fnt.glyph_widths[n++] = jsn_int(jt);
        }
        os_spmem_pop();
        return fnt;
}

int fntlength_px(fnt_s *font, fntchar_s *chars, int l)
{
        int len_max = 0;
        int len     = 0;
        for (int n = 0; n < l; n++) {
                fntchar_s c = chars[n];
                if (c.glyphID == FNT_GLYPH_NEWLINE) {
                        len = 0;
                        continue;
                }
                len += font->glyph_widths[c.glyphID];
                len_max = len > len_max ? len : len_max;
        }
        return len_max;
}

int fntlength_px_ascii(fnt_s *font, const char *txt, int l)
{
        NOT_IMPLEMENTED
        os_spmem_push();
        return 0;
        os_spmem_pop();
}

fntchar_s fntchar_from_glyphID(int glyphID)
{
        fntchar_s c = {glyphID, 0};
        return c;
}

bool32 fntstr_append_glyph(fntstr_s *f, int glyphID)
{
        if (f->n >= f->c) return 0;
        fntchar_s c      = {glyphID, 0};
        f->chars[f->n++] = c;
        return 1;
}

int fntstr_append_ascii(fntstr_s *f, const char *txt)
{
        int i;
        for (i = 0; txt[i] != '\0'; i++) {
                int c = (int)txt[i];
                if (!fntstr_append_glyph(f, c))
                        return i;
        }
        return i;
}

int fntstr_len(fntstr_s *f)
{
        return f->n;
}

void fntstr_apply_effect(fntstr_s *f, int from, int to,
                         int effect, int tick)
{
        int n1 = from >= 0 ? from : 0;
        int n2 = to < f->n ? to : f->n;
        switch (effect) {
        case 0:
                for (int n = n1; n < n2; n++) {
                        f->chars[n].effectID = 0;
                }
                break;
        }
}

void gfx_tex_clr(tex_s t)
{
        size_t s = sizeof(u8) * t.w_byte * t.h * (t.mk ? 2 : 1);
        os_memclr4(t.px, s);
}

static inline int i_gfx_peek_px(tex_s t, int x, int y)
{
        if (!(0 <= x && x < t.w && 0 <= y && y < t.h)) return 0;

        int i = t.px[(x >> 3) + y * t.w_byte];
        return ((i & (0x80 >> (x & 7))) > 0);
}

static inline void i_gfx_peek(tex_s t, int x, int y, int *px, int *op)
{
        if (!(0 <= x && x < t.w && 0 <= y && y < t.h)) {
                *px = 0;
                *op = 0;
                return;
        }

        int i = (x >> 3) + y * t.w_byte;
        int m = 0x80 >> (x & 7);
        *px   = (t.px[i] & m) > 0;
        *op   = (t.mk[i] & m);
}

// TODO: implement mode
static inline void i_gfx_put_px(tex_s t, int x, int y, int col, int mode)
{
        if (!(0 <= x && x < t.w && 0 <= y && y < t.h)) return;
        int s = 0x80 >> (x & 7);
        if ((g_os.dstpat.p[y & 7] & s) == 0) return;
        int i   = (x >> 3) + y * t.w_byte;
        t.px[i] = (col ? t.px[i] | s : t.px[i] & ~s); // set or clear bit
        if (t.mk) t.mk[i] |= s;
}

static inline void i_gfx_fill_span(int y, int xa, int xb, int col)
{
        tex_s dst = g_os.dst;
        if (!(0 <= y && y < dst.h)) return;
        if (xa == xb) {
                i_gfx_put_px(dst, xa, y, col, 0);
                return;
        }

        int x1 = MAX(0, xa);
        int x2 = MIN(dst.w - 1, xb);
        if (x2 <= x1) return;

        u32 pt = g_os.dstpat.p[y & 7];
        if (!pt) return;

        int  yd = y * dst.w_word;
        int  b1 = (x1 >> 5) + yd;
        int  b2 = (x2 >> 5) + yd;
        u32  sp = col ? 0xFFFFFFFFu : 0;
        u32 *dp = (u32 *)dst.px;
        u32 *dm = (u32 *)dst.mk;

        for (int b = b1; b <= b2; b++) {
                int uu = (b == b1 ? x1 & 31 : 0);
                int vv = (b == b2 ? x2 & 31 : 31);
                u32 sm = (0xFFFFFFFFu >> uu) & ~(0x7FFFFFFFu >> vv);
                u32 t0 = endian_u32(sm) & pt;
                u32 d0 = dp[b];
                switch (col) {
                case GFX_COL_BLACK:
                        break;
                case GFX_COL_WHITE:
                        break;
                case GFX_COL_CLEAR:
                        break;
                case GFX_COL_NXOR:
                        sp = ~d0;
                        break;
                }
                dp[b] = (d0 & ~t0) | (sp & t0);
                if (!dm) continue;
                dm[b] |= t0;
        }
}

void gfx_px(int x, int y, int col)
{
        i_gfx_put_px(g_os.dst, x, y, col, 0);
}

void gfx_set_pattern(gfx_pattern_s pat)
{
        g_os.dstpat = pat;
}

void gfx_reset_pattern()
{
        gfx_set_pattern(g_gfx_patterns[GFX_PATTERN_FULL]);
}

void gfx_set_inverted(bool32 inv)
{
#ifdef TARGET_PD
        PD->display->setInverted(inv);
#else
        g_os.inverted = inv;
#endif
}

void gfx_draw_to(tex_s tex)
{
        g_os.dst = tex;
}

void gfx_draw_to_ID(int ID)
{
        gfx_draw_to(tex_get(ID));
}

static inline u32 i_gfx_pattern_u32_from_u8(int b)
{
        return (((u32)b << 24) | ((u32)b << 16) | ((u32)b << 8) | (u32)b);
}

gfx_pattern_s gfx_pattern_set_8x8(int p0, int p1, int p2, int p3,
                                  int p4, int p5, int p6, int p7)
{
        gfx_pattern_s p = {
            i_gfx_pattern_u32_from_u8(p0), i_gfx_pattern_u32_from_u8(p1),
            i_gfx_pattern_u32_from_u8(p2), i_gfx_pattern_u32_from_u8(p3),
            i_gfx_pattern_u32_from_u8(p4), i_gfx_pattern_u32_from_u8(p5),
            i_gfx_pattern_u32_from_u8(p6), i_gfx_pattern_u32_from_u8(p7)};
        return p;
}

/* fast sprite drawing routine for untransformed sprites
 * blits 32 pixelbits in one loop
 */
void gfx_sprite_ext(tex_s src, v2_i32 pos, rec_i32 rs, int flags, int mode)
{
        tex_s dst = g_os.dst;
        int   xx  = (flags & GFX_SPRITE_X ? -1 : +1); // XY flipping factors
        int   yy  = (flags & GFX_SPRITE_Y ? -1 : +1);
        int   xa  = rs.x + (flags & GFX_SPRITE_X ? pos.x + rs.w - 1 : -pos.x);
        int   ya  = rs.y + (flags & GFX_SPRITE_Y ? pos.y + rs.h - 1 : -pos.y);
        int   x1  = MAX(pos.x, 0); // pixel bounds on canvas
        int   y1  = MAX(pos.y, 0);
        int   x2  = MIN(pos.x + rs.w, dst.w) - 1;
        int   y2  = MIN(pos.y + rs.h, dst.h) - 1;
        int   b1  = (x1 >> 5);                         // first dst byte in x
        int   b2  = (x2 >> 5);                         // last dst byte in x
        int   s0  = (32 - ((pos.x - rs.x) & 31)) & 31; // alignment shift
        int   s1  = (32 - s0) & 31;

        // calc pixel coord in source image from canvas pixel coord
        // src_x = dst_x * xx + xa   | xx and yy are either +1 or -1
        // dst_x = (src_x - xa) * xx | xx and yy are either +1 or -1
        for (int y = y1; y <= y2; y++) {
                int  ys = (y * yy + ya) * src.w_word; // source y coord cache
                int  jj = b1 + y * dst.w_word;
                u32  pt = g_os.dstpat.p[y & 7]; // pattern mask
                u32 *dp = &((u32 *)dst.px)[jj];
                u32 *dm = dst.mk ? &((u32 *)dst.mk)[jj] : NULL;
                for (int b = b1; b <= b2; b++) {
                        int xs0 = ((b << 5)) * xx + xa;
                        int xs1 = ((b << 5) + 31) * xx + xa;
                        u32 t   = 0; // rendered pixels mask
                        u32 p   = 0; // black and white bits
                        if (!(flags & GFX_SPRITE_X)) {
                                int i0 = (xs0 >> 5) + ys;
                                int i1 = (xs1 >> 5) + ys;
                                p |= endian_u32(((u32 *)src.px)[i0]) << s0;
                                p |= endian_u32(((u32 *)src.px)[i1]) >> s1;
                                t |= endian_u32(((u32 *)src.mk)[i0]) << s0;
                                t |= endian_u32(((u32 *)src.mk)[i1]) >> s1;
                        } else { // flipped X
                                // xs0 and xs1 are logically swapped
                                xs1 = MAX(xs1, 0);
                                xs0 = MIN(xs0, src.w - 1);

                                // construct masks manually...
                                // optimize at some point?
                                for (int xs = xs1; xs <= xs0; xs++) {
                                        int i0 = (xs >> 5) + ys;
                                        u32 sp = endian_u32(((u32 *)src.px)[i0]);
                                        u32 sm = endian_u32(((u32 *)src.mk)[i0]);
                                        int xd = (xs - xa) * xx;
                                        u32 ms = 0x80000000U >> (xs & 31);
                                        u32 md = 0x80000000U >> (xd & 31);
                                        if (!(sm & ms)) continue;
                                        t |= md;
                                        if (sp & ms) p |= md;
                                }
                        }

                        if (b == b1) // mask off pixels out of left drawing boundary
                                t &= ((0xFFFFFFFFU >> (x1 & 31)));
                        if (b == b2) // mask off pixels out of right drawing boundary
                                t &= ~(0x7FFFFFFFU >> (x2 & 31));
                        t     = endian_u32(t & pt);
                        p     = endian_u32(p);
                        u32 d = *dp;
                        switch (mode) {
                        case GFX_SPRITE_INV: p = ~p; // fallthrough
                        case GFX_SPRITE_COPY:
                                d = (d & ~t) | (t & p);
                                break;
                        case GFX_SPRITE_XOR: p = ~p; // fallthrough
                        case GFX_SPRITE_NXOR:
                                d = (d & ~t) | ((d ^ p) & t);
                                break;
                        case GFX_SPRITE_WHITE_TRANSPARENT: t &= p; // fallthrough
                        case GFX_SPRITE_FILL_BLACK:
                                d |= t;
                                break;
                        case GFX_SPRITE_BLACK_TRANSPARENT: t &= ~p; // fallthrough
                        case GFX_SPRITE_FILL_WHITE:
                                d &= ~t;
                                break;
                        }

                        *dp++ = d;
                        if (!dm) continue;
                        *dm++ |= t;
                }
        }
}

void gfx_sprite_mode(tex_s src, v2_i32 pos, rec_i32 rs, int mode)
{
        gfx_sprite_ext(src, pos, rs, 0, mode);
}

void gfx_sprite_fast(tex_s src, v2_i32 pos, rec_i32 rs)
{
        gfx_sprite_mode(src, pos, rs, 0);
}

void gfx_tr_sprite_fast(texregion_s src, v2_i32 pos)
{
        gfx_sprite_mode(src.t, pos, src.r, 0);
}

void gfx_tr_sprite_mode(texregion_s src, v2_i32 pos, int mode)
{
        gfx_sprite_mode(src.t, pos, src.r, mode);
}

void gfx_tr_sprite_flip(texregion_s src, v2_i32 pos, int flags)
{
        gfx_sprite_ext(src.t, pos, src.r, flags, 0);
}

void gfx_sprite_matrix(tex_s src, v2_i32 pos, rec_i32 rs, i32 m[4])
{
        tex_s dst = g_os.dst;
        for (int y = 0; y < dst.h; y++) {
                for (int x = 0; x < dst.w; x++) {
                        int tx = x + rs.x - pos.x;
                        int ty = y + rs.y - pos.y;
                        i32 xx = ((tx * m[0] + ty * m[1]) >> 12);
                        i32 yy = ((tx * m[2] + ty * m[3]) >> 12);
                        if (!(xx >= rs.x && yy >= rs.y &&
                              xx < rs.x + rs.w && yy < rs.y + rs.h)) continue;
                        i_gfx_put_px(dst, x, y, i_gfx_peek_px(src, xx, yy), 0);
                }
        }
}

void gfx_rec_fill(rec_i32 r, int col)
{
        tex_s dst = g_os.dst;
        int   ya  = r.y;
        int   yb  = r.y + r.h;
        int   y1  = MAX(0, ya);
        int   y2  = MIN(dst.h, yb);
        for (int y = y1; y < y2; y++) {
                i_gfx_fill_span(y, r.x, r.x + r.w, col);
        }
}

void gfx_tri_fill(v2_i32 p0, v2_i32 p1, v2_i32 p2, int col)
{
        tex_s  dst = g_os.dst;
        v2_i32 t0 = p0, t1 = p1, t2 = p2;
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
        i32 y0 = MAX(0, t0.y);
        i32 y1 = MIN(dst.h - 1, t1.y);
        i32 y2 = MIN(dst.h - 1, t2.y);
        for (int y = y0; y <= y1; y++) {
                i32 x1 = t0.x + (d0 * (y - t0.y)) / th;
                i32 x2 = t0.x + (d1 * (y - t0.y)) / h1;
                if (x2 < x1) SWAP(i32, x1, x2);
                i_gfx_fill_span(y, x1, x2, col);
        }

        for (int y = y1; y <= y2; y++) {
                i32 x1 = t0.x + (d0 * (y - t0.y)) / th;
                i32 x2 = t1.x + (d2 * (y - t1.y)) / h2;
                if (x2 < x1) SWAP(i32, x1, x2);
                i_gfx_fill_span(y, x1, x2, col);
        }
}

void gfx_line(int x0, int y0, int x1, int y1, int col)
{
        tex_s dst = g_os.dst;
        int   dx = +ABS(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int   dy = -ABS(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int   er = dx + dy;
        int   xi = x0;
        int   yi = y0;
        while (1) {
                i_gfx_put_px(dst, xi, yi, col, 0);
                if (xi == x1 && yi == y1) break;
                int e2 = er * 2;
                if (e2 >= dy) { er += dy, xi += sx; }
                if (e2 <= dx) { er += dx, yi += sy; }
        }
}

// naive thick line, works for now
void gfx_line_thick(int x0, int y0, int x1, int y1, int r, int col)
{
        tex_s dst = g_os.dst;
        int   dx = +ABS(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int   dy = -ABS(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int   er = dx + dy;
        int   xi = x0;
        int   yi = y0;
        int   r2 = r * r;
        while (1) {
                for (int y = 0; y <= +r; y++) {
                        for (int x = r; x >= 0; x--) {
                                if (x * x + y * y > r2) continue;
                                i_gfx_fill_span(yi - y, xi - x, xi + x, col);
                                i_gfx_fill_span(yi + y, xi - x, xi + x, col);
                                break;
                        }
                }

                if (xi == x1 && yi == y1) break;
                int e2 = er * 2;
                if (e2 >= dy) { er += dy, xi += sx; }
                if (e2 <= dx) { er += dx, yi += sy; }
        }
}

void gfx_text_ascii(fnt_s *font, const char *txt, int x, int y)
{
        int len = 1;
        for (int i = 0; txt[i] != '\0'; i++) {
                len++;
        }

        os_spmem_push();
        fntstr_s str = fntstr_create(len, os_spmem_alloc);
        fntstr_append_ascii(&str, txt);
        gfx_text(font, &str, x, y);
        os_spmem_pop();
}

void gfx_text(fnt_s *font, fntstr_s *str, int x, int y)
{
        tex_s fonttex = font->tex;
        gfx_draw_to(tex_get(TEXID_DISPLAY));

        v2_i32 p = {x, y};
        for (int i = 0; i < fntstr_len(str); i++) {
                fntchar_s c   = str->chars[i];
                int       cID = (int)c.glyphID;
                int       gy  = cID >> 5; // 32 glyphs in a row
                int       gx  = cID & 31;
                rec_i32   r   = {gx * font->gridw,
                                 gy * font->gridh,
                                 font->gridw,
                                 font->gridh};

                gfx_sprite_fast(fonttex, p, r);
                p.x += font->glyph_widths[cID];
        }
}

void gfx_text_glyphs(fnt_s *font, fntchar_s *chars, int l, int x, int y)
{
        tex_s fonttex = font->tex;
        gfx_draw_to(tex_get(TEXID_DISPLAY));

        v2_i32 p = {x, y};
        for (int i = 0; i < l; i++) {
                fntchar_s c   = chars[i];
                int       cID = (int)c.glyphID;
                int       gy  = cID >> 5; // 32 glyphs in a row
                int       gx  = cID & 31;
                rec_i32   r   = {gx * font->gridw,
                                 gy * font->gridh,
                                 font->gridw,
                                 font->gridh};

                v2_i32 pp = p;
                switch (c.effectID) {
                case FNT_EFFECT_SHAKE:
                        pp.x += rng_range(-1, +1);
                        pp.y += rng_range(-1, +1);
                        break;
                case FNT_EFFECT_WAVE:
                        pp.y += sin_q16((os_tick() << 13) + (i << 16)) >> 15;
                        break;
                }

                gfx_sprite_fast(fonttex, pp, r);
                p.x += font->glyph_widths[cID];
        }
}

// 2 bits of subpixel precision
// 0 ... 4
// V     V
// -------
// |     |
// |     |
// -------
// to texture map the whole area
// from 0 incl to 7 incl (width of 8)
//
// 0               (7+1)<<2 -> right border of the pixel
// V               V
// |0|1|2|3|4|5|6|7|8|...
void gfx_sprite_tri_affine(tex_s src, v2_i32 tri[3], v2_i32 tex[3])
{
        v2_i32 t12 = v2_sub(tri[2], tri[1]);
        v2_i32 t20 = v2_sub(tri[0], tri[2]);
        v2_i32 t01 = v2_sub(tri[1], tri[0]);
        i32    den = v2_crs(t01, t20);
        if (den == 0) return;
        if (den < 0) {
                den = -den;
                t12 = v2_inv(t12);
                t20 = v2_inv(t20);
                t01 = v2_inv(t01);
        }

        tex_s     dst  = g_os.dst;
        // min and max bounds on screen
        v2_i32    pmin = v2_shr(v2_min(tri[0], v2_min(tri[1], tri[2])), 2);
        v2_i32    pmax = v2_shr(v2_max(tri[0], v2_max(tri[1], tri[2])), 2);
        // screen bounds
        v2_i32    scr1 = v2_max(pmin, (v2_i32){0, 0});
        v2_i32    scr2 = v2_min(pmax, (v2_i32){dst.w - 1, dst.h - 1});
        // pixel midpoint for subpixel sampling
        v2_i32    xya  = v2_add(v2_shl(scr1, 2), (v2_i32){2, 2});
        // interpolators
        i32       u0   = v2_crs(v2_sub(xya, tri[1]), t12);
        i32       v0   = v2_crs(v2_sub(xya, tri[2]), t20);
        i32       w0   = v2_crs(v2_sub(xya, tri[0]), t01);
        i32       ux = t12.y << 2, uy = -t12.x << 2;
        i32       vx = t20.y << 2, vy = -t20.x << 2;
        i32       wx = t01.y << 2, wy = -t01.x << 2;
        // quick divider
        u32       dv  = 1u + ((u32)den << 2); // +1 -> prevent overflow of texcoord
        div_u32_s div = div_u32_create(dv);

        for (int y = scr1.y; y <= scr2.y; y++) {
                i32 u = u0, v = v0, w = w0;
                for (int x = scr1.x; x <= scr2.x; x++) {
                        if ((u | v | w) >= 0) {
                                u32 tu = mul_u32(u, tex[0].x) +
                                         mul_u32(v, tex[1].x) +
                                         mul_u32(w, tex[2].x);
                                u32 tv = mul_u32(u, tex[0].y) +
                                         mul_u32(v, tex[1].y) +
                                         mul_u32(w, tex[2].y);
                                // only works if numerator is positiv!
                                int tx = div_u32_do(tu, div);
                                int ty = div_u32_do(tv, div);
                                int px, mk;
                                i_gfx_peek(src, tx, ty, &px, &mk);
                                i_gfx_put_px(dst, x, y, px, 0);
                        }
                        u += ux, v += vx, w += wx;
                }
                u0 += uy, v0 += vy, w0 += wy;
        }
}

const gfx_pattern_s g_gfx_patterns[NUM_GFX_PATTERN] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    //
    0x88888888U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x88888888U, 0x00000000U, 0x00000000U, 0x00000000U,
    //
    0x22222222U, 0x00000000U, 0x88888888U, 0x00000000U,
    0x22222222U, 0x00000000U, 0x88888888U, 0x00000000U,
    //
    0x22222222U, 0x00000000U, 0xAAAAAAAAU, 0x00000000U,
    0x22222222U, 0x00000000U, 0xAAAAAAAAU, 0x00000000U,
    //
    0xAAAAAAAAU, 0x00000000U, 0xAAAAAAAAU, 0x00000000U,
    0xAAAAAAAAU, 0x00000000U, 0xAAAAAAAAU, 0x00000000U,
    //
    0xAAAAAAAAU, 0x44444444U, 0xAAAAAAAAU, 0x00000000U,
    0xAAAAAAAAU, 0x44444444U, 0xAAAAAAAAU, 0x00000000U,
    //
    0xAAAAAAAAU, 0x44444444U, 0xAAAAAAAAU, 0x11111111U,
    0xAAAAAAAAU, 0x44444444U, 0xAAAAAAAAU, 0x11111111U,
    //
    0xAAAAAAAAU, 0x55555555U, 0xAAAAAAAAU, 0x11111111U,
    0xAAAAAAAAU, 0x55555555U, 0xAAAAAAAAU, 0x11111111U,
    //
    0xAAAAAAAAU, 0x55555555U, 0xAAAAAAAAU, 0x55555555U, // checkerboard
    0xAAAAAAAAU, 0x55555555U, 0xAAAAAAAAU, 0x55555555U,
    //
    // from here on basically only inverted
    0x55555555U, 0xAAAAAAAAU, 0x55555555U, 0xEEEEEEEEU,
    0x55555555U, 0xAAAAAAAAU, 0x55555555U, 0xEEEEEEEEU,
    //
    0x55555555U, 0xBBBBBBBBU, 0x55555555U, 0xEEEEEEEEU,
    0x55555555U, 0xBBBBBBBBU, 0x55555555U, 0xEEEEEEEEU,
    //
    0x55555555U, 0xBBBBBBBBU, 0x55555555U, 0xFFFFFFFFU,
    0x55555555U, 0xBBBBBBBBU, 0x55555555U, 0xFFFFFFFFU,
    //
    0x55555555U, 0xFFFFFFFFU, 0x55555555U, 0xFFFFFFFFU,
    0x55555555U, 0xFFFFFFFFU, 0x55555555U, 0xFFFFFFFFU,
    //
    0xDDDDDDDDU, 0xFFFFFFFFU, 0x55555555U, 0xFFFFFFFFU,
    0xDDDDDDDDU, 0xFFFFFFFFU, 0x55555555U, 0xFFFFFFFFU,
    //
    0xDDDDDDDDU, 0xFFFFFFFFU, 0x77777777U, 0xFFFFFFFFU,
    0xDDDDDDDDU, 0xFFFFFFFFU, 0x77777777U, 0xFFFFFFFFU,
    //
    0x77777777U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
    0x77777777U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
    //
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
    //
};