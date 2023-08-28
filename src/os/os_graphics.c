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

static inline void i_gfx_put_px(tex_s t, int x, int y, int col, int mode)
{
        if (!(0 <= x && x < t.w && 0 <= y && y < t.h)) return;

        int i = (x >> 3) + y * t.w_byte;
        int s = 0x80 >> (x & 7);
        if (col)
                t.px[i] |= s; // set bit
        else
                t.px[i] &= ~s; // clear bit
        if (t.mk) t.mk[i] |= s;
}

void gfx_px(int x, int y, int col)
{
        i_gfx_put_px(g_os.dst, x, y, col, 0);
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

#if 1 // archived

/* this is a naive image drawing routine
 * can flip an image x, y and diagonal (rotate 90 degree)
 * could be hugely improved by figuring out a way to put multiple
 * pixels at once
 */
void gfx_sprite_flip(tex_s src, v2_i32 pos, rec_i32 rs, int flags)
{
        int xx, yy, xy, yx;
        int ff = flags & 7;
        int xa = rs.x;
        int ya = rs.y;
        switch (ff) {
        case 0: xx = +1, yy = +1, xy = yx = 0; break;
        case 1: xx = +1, yy = -1, xy = yx = 0, ya += rs.h - 1; break;                 /* flipy */
        case 2: xx = -1, yy = +1, xy = yx = 0, xa += rs.w - 1; break;                 /* flipx */
        case 3: xx = -1, yy = -1, xy = yx = 0, xa += rs.w - 1, ya += rs.h - 1; break; /* flip xy / rotate 180 */
        case 4: xy = +1, yx = +1, xx = yy = 0; break;                                 /* rotate 90 cw, then flip x */
        case 5: xy = -1, yx = +1, xx = yy = 0, xa += rs.w - 1; break;                 /* rotate 90 ccw */
        case 6: xy = +1, yx = -1, xx = yy = 0, ya += rs.h - 1; break;                 /* rotate 90 cw */
        case 7: xy = -1, yx = -1, xx = yy = 0, xa += rs.w - 1, ya += rs.h - 1; break; /* rotate 90 cw, then flip y */
        }

        tex_s dst = g_os.dst;
        int   zx  = dst.w - pos.x;
        int   zy  = dst.h - pos.y;
        int   dx  = ff >= 4 ? rs.h : rs.w;
        int   dy  = ff >= 4 ? rs.w : rs.h;
        int   x2  = dx <= zx ? dx : zx;
        int   y2  = dy <= zy ? dy : zy;
        int   x1  = 0 >= -pos.x ? 0 : -pos.x;
        int   y1  = 0 >= -pos.y ? 0 : -pos.y;

        for (int y = y1; y < y2; y++) {
                for (int x = x1; x < x2; x++) {
                        int xp = x + pos.x;
                        int yp = y + pos.y;
                        int xs = x * xx + y * xy + xa;
                        int ys = x * yx + y * yy + ya;
                        int px, op;
                        i_gfx_peek(src, xs, ys, &px, &op);
                        if (op) i_gfx_put_px(dst, xp, yp, px, 0);
                }
        }
}
#endif

gfx_pattern_s gfx_pattern_set_8x8(int p0, int p1, int p2, int p3,
                                  int p4, int p5, int p6, int p7)
{
        u32 t[8] = {
            ((u32)p0 << 24) | ((u32)p0 << 16) | ((u32)p0 << 8) | (u32)p0,
            ((u32)p1 << 24) | ((u32)p1 << 16) | ((u32)p1 << 8) | (u32)p1,
            ((u32)p2 << 24) | ((u32)p2 << 16) | ((u32)p2 << 8) | (u32)p2,
            ((u32)p3 << 24) | ((u32)p3 << 16) | ((u32)p3 << 8) | (u32)p3,
            ((u32)p4 << 24) | ((u32)p4 << 16) | ((u32)p4 << 8) | (u32)p4,
            ((u32)p5 << 24) | ((u32)p5 << 16) | ((u32)p5 << 8) | (u32)p5,
            ((u32)p6 << 24) | ((u32)p6 << 16) | ((u32)p6 << 8) | (u32)p6,
            ((u32)p7 << 24) | ((u32)p7 << 16) | ((u32)p7 << 8) | (u32)p7};

        gfx_pattern_s p = {t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7],
                           t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7],
                           t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7],
                           t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7]};
        return p;
}

/* fast sprite drawing routine for untransformed sprites
 * blits 32 pixelbits in one loop
 */
void gfx_sprite_ext(tex_s src, v2_i32 pos, rec_i32 rs, int mode, gfx_pattern_s pat)
{
        tex_s dst        = g_os.dst;
        int   zx         = dst.w - pos.x;
        int   zy         = dst.h - pos.y;
        int   x1         = rs.x + MAX(0, -pos.x);
        int   y1         = rs.y + MAX(0, -pos.y);
        int   x2         = rs.x + MIN(rs.w, zx) - 1;
        int   y2         = rs.y + MIN(rs.h, zy) - 1;
        int   cc         = pos.x - rs.x;
        int   s1         = 31 & (32 - (cc & 31)); // relative word alignment
        int   s0         = 32 - s1;
        int   b1         = x1 >> 5;
        int   b2         = x2 >> 5;
        u32 *restrict dp = (u32 *)dst.px;
        u32 *restrict dm = (u32 *)dst.mk;

        for (int y = y1; y <= y2; y++) {
                int yd           = (y + pos.y - rs.y) * dst.w_word;
                int ii           = b1 + y * src.w_word;
                u32 *restrict zm = &((u32 *)src.mk)[ii];
                u32 *restrict zp = &((u32 *)src.px)[ii];
                for (int b = b1; b <= b2; b++) {
                        int uu = (b == b1 ? x1 & 31 : 0);
                        int vv = (b == b2 ? x2 & 31 : 31);
                        u32 mm = (0xFFFFFFFFu >> uu) & ~(0x7FFFFFFFu >> vv);
                        u32 sm = endian_u32(*zm++) & mm;
                        u32 sp = endian_u32(*zp++);

                        sm &= pat.p[y & 31];

                        u32 t0 = endian_u32(sm >> s0);
                        u32 p0 = endian_u32(sp >> s0);
                        u32 t1 = endian_u32(sm << s1);
                        u32 p1 = endian_u32(sp << s1);

                        int j0 = (((b << 5) + cc + uu) >> 5) + yd; // <- +uu -> prevent underflow under certain conditions!
                        int j1 = (((b << 5) + cc + 31) >> 5) + yd;
                        u32 d0 = dp[j0];
                        u32 d1 = dp[j1];
                        switch (mode) {
                        case GFX_SPRITE_INV:
                                p0 = ~p0;
                                p1 = ~p1; // fallthrough
                        case GFX_SPRITE_COPY:
                                d0 = (d0 & ~t0) | (p0 & t0);
                                d1 = (d1 & ~t1) | (p1 & t1);
                                break;
                        case GFX_SPRITE_XOR:
                                p0 = ~p0;
                                p1 = ~p1; // fallthrough
                        case GFX_SPRITE_NXOR:
                                d0 = (d0 & ~t0) | ((d0 ^ p0) & t0);
                                d1 = (d1 & ~t1) | ((d1 ^ p1) & t1);
                                break;
                        case GFX_SPRITE_WHITE_TRANSPARENT:
                                t0 &= p0;
                                t1 &= p1; // fallthrough
                        case GFX_SPRITE_FILL_BLACK:
                                d0 |= t0;
                                d1 |= t1;
                                break;
                        case GFX_SPRITE_BLACK_TRANSPARENT:
                                t0 &= ~p0;
                                t1 &= ~p1; // fallthrough
                        case GFX_SPRITE_FILL_WHITE:
                                d0 &= ~t0;
                                d1 &= ~t1;
                                break;
                        }
                        dp[j0] = d0;
                        dp[j1] = d1;

                        if (dm) {
                                dm[j0] |= t0;
                                dm[j1] |= t1;
                        }
                }
        }
}

/* fast sprite drawing routine for untransformed sprites
 * blits 32 pixelbits in one loop
 */
void gfx_sprite_mode(tex_s src, v2_i32 pos, rec_i32 rs, int mode)
{
        static const gfx_pattern_s pattern = {
            0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
            0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
            0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
            0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
            0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
            0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
            0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
            0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
            //
        };
        gfx_sprite_ext(src, pos, rs, mode, pattern);
}

// TODO: doesnt work when drawing from a sprite without a mask
//
/* fast sprite drawing routine for untransformed sprites
 * blits 32 pixelbits in one loop
 */
void gfx_sprite_fast(tex_s src, v2_i32 pos, rec_i32 rs)
{
        gfx_sprite_mode(src, pos, rs, 0);
}

void gfx_tr_sprite_fast(texregion_s src, v2_i32 pos)
{
        gfx_sprite_mode(src.t, pos, src.r, 0);
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

void gfx_sprite_squished(tex_s src, i32 x, i32 y1, i32 y2, rec_i32 rs)
{
        tex_s dst = g_os.dst;
        i32   hh  = y2 - y1;
        if (hh == 0) return;
        for (int y = y1; y <= y2; y++) {
                i32     sy = (rs.h * (y - y1)) / hh + rs.y;
                rec_i32 sr = {rs.x, sy, rs.w, 1};
                gfx_sprite_fast(src, (v2_i32){x, y}, sr);
        }
}

void gfx_rec_fill(rec_i32 r, int col)
{
        tex_s   dst = g_os.dst;
        rec_i32 ri;
        rec_i32 rd = {0, 0, dst.w, dst.h};
        if (!intersect_rec(rd, r, &ri)) return;
        int zx           = dst.w - ri.x;
        int zy           = dst.h - ri.y;
        int x1           = (0 >= -ri.x ? 0 : -ri.x);
        int y1           = (0 >= -ri.y ? 0 : -ri.y);
        int x2           = (ri.w <= zx ? ri.w : zx) - 1;
        int y2           = (ri.h <= zy ? ri.h : zy) - 1;
        int cc           = ri.x;
        int s1           = 31 & (32 - (cc & 31)); // relative word alignment
        int s0           = 32 - s1;
        int b1           = x1 >> 5;
        int b2           = x2 >> 5;
        u32 sp           = col ? 0xFFFFFFFFu : 0;
        u32 p0           = endian_u32(sp >> s0);
        u32 p1           = endian_u32(sp << s1);
        u32 *restrict dp = (u32 *)dst.px;
        u32 *restrict dm = (u32 *)dst.mk;
        for (int y = y1; y <= y2; y++) {
                int yd = (y + ri.y) * dst.w_word;
                for (int b = b1; b <= b2; b++) {
                        int uu = (b == b1 ? x1 & 31 : 0);
                        int vv = (b == b2 ? x2 & 31 : 31);
                        u32 sm = (0xFFFFFFFFu >> uu) & ~(0x7FFFFFFFu >> vv);
                        u32 t0 = endian_u32(sm >> s0);
                        u32 t1 = endian_u32(sm << s1);
                        int j0 = (((b << 5) + cc + uu) >> 5) + yd; // <- +uu -> prevent underflow under certain conditions!
                        int j1 = (((b << 5) + cc + 31) >> 5) + yd;
                        u32 d0 = dp[j0];
                        u32 d1 = dp[j1];
                        dp[j0] = (d0 & ~t0) | (p0 & t0);
                        dp[j1] = (d1 & ~t1) | (p1 & t1);

                        if (dm) {
                                dm[j0] |= t0;
                                dm[j1] |= t1;
                        }
                }
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
                for (int y = -r; y <= +r; y++) {
                        for (int x = -r; x <= +r; x++) {
                                if (x * x + y * y > r2) continue;
                                i_gfx_put_px(dst, xi + x, yi + y, col, 0);
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