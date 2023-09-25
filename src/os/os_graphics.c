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
        int  l = t.w_word * t.h * (t.mk ? 2 : 1);
        u32 *p = (u32 *)t.px;
        for (int n = 0; n < l; n++) {
                *p++ = 0;
        }
}

static inline void i_gfx_apply_px(u32 *dp, u32 *dm, u32 pt, u32 pp, u32 tt, int m)
{
        u32 t = bswap32(tt & pt); // mask off pattern pixels
        u32 p = bswap32(pp);

        switch (m) {
        case GFX_MODE_INV: p = ~p; // fallthrough
        case GFX_MODE_CPY: *dp = (*dp & ~t) | (p & t); break;
        case GFX_MODE_XOR: p = ~p; // fallthrough
        case GFX_MODE_NXR: *dp = (*dp & ~t) | ((*dp ^ p) & t); break;
        case GFX_MODE_W_T: t &= p; // fallthrough
        case GFX_MODE_B_F: *dp |= t; break;
        case GFX_MODE_B_T: t &= ~p; // fallthrough
        case GFX_MODE_W_F: *dp &= ~t; break;
        }

        if (dm) { // write opaque pixel info if mask is present
                *dm |= t;
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

// TODO: implement mode
static inline void i_gfx_put_px(gfx_context_s ctx, int x, int y)
{
        if (!(0 <= x && x < ctx.dst.w && 0 <= y && y < ctx.dst.h)) return;

        int  j  = (x >> 5) + y * ctx.dst.w_word;
        u32 *dp = &((u32 *)ctx.dst.px)[j];
        u32 *dm = ctx.dst.mk ? &((u32 *)ctx.dst.mk)[j] : NULL;

        u32 s  = 0x80000000U >> (x & 31);
        u32 pt = ctx.pat.p[y & 7];
        u32 px;
        u32 tt;

        switch (ctx.col) {
        case GFX_PRIM_BW:
                px = (pt & s ? s : 0);
                tt = s;
                break;
        case GFX_PRIM_BLACK_TRANSPARENT:
                px = s;
                tt = px;
                break;
        case GFX_PRIM_WHITE_TRANSPARENT:
                px = 0;
                tt = (pt & s ? s : 0);
                break;
        }

        i_gfx_apply_px(dp, dm, pt, px, tt, ctx.sprmode);
}

// TODO: implement mode
static inline void i_gfx_px_(gfx_context_s ctx, int x, int y, int col)
{
        if (!(0 <= x && x < ctx.dst.w && 0 <= y && y < ctx.dst.h)) return;
        int s = 0x80 >> (x & 7);
        if ((ctx.pat.p[y & 7] & s) == 0) return;
        int i         = (x >> 3) + y * ctx.dst.w_byte;
        ctx.dst.px[i] = (col ? ctx.dst.px[i] | s : ctx.dst.px[i] & ~s); // set or clear bit
        if (ctx.dst.mk) ctx.dst.mk[i] |= s;
}

// TODO: implement mode
static inline void i_gfx_px_shape(gfx_context_s ctx, int x, int y)
{
        if (!(0 <= x && x < ctx.dst.w && 0 <= y && y < ctx.dst.h)) return;
        int s = 0x80 >> (x & 7);
        if ((ctx.pat.p[y & 7] & s) == 0) return;
        int i         = (x >> 3) + y * ctx.dst.w_byte;
        ctx.dst.px[i] = (ctx.col ? ctx.dst.px[i] | s : ctx.dst.px[i] & ~s); // set or clear bit
        if (ctx.dst.mk) ctx.dst.mk[i] |= s;
}

static inline void i_gfx_span_shape(gfx_context_s ctx, int y, int xa, int xb)
{
        if (!(0 <= y && y < ctx.dst.h)) return;

        int x1 = max_i(0, xa);
        int x2 = min_i(ctx.dst.w - 1, xb);
        if (x2 <= x1) return;
#if 0
        for (int xxx = x1; xxx <= x2; xxx++) {
                i_gfx_put_px(ctx, xxx, y);
        }
        return;
#endif

        u32 pt = ctx.pat.p[y & 7];
        if (!pt) return;

        int  b1 = (x1 >> 5) + y * ctx.dst.w_word;
        int  b2 = (x2 >> 5) + y * ctx.dst.w_word;
        u32 *dp = &((u32 *)ctx.dst.px)[b1];
        u32 *dm = ctx.dst.mk ? &((u32 *)ctx.dst.mk)[b1] : NULL;
        u32  ul = bswap32(((0xFFFFFFFFU >> (x1 & 31))));
        u32  ur = bswap32(~(0x7FFFFFFFU >> (x2 & 31)));
        for (int b = b1; b <= b2; b++) {
                u32 t = pt;
                if (b == b1) t &= ul;
                if (b == b2) t &= ur;

                u32 d = *dp;
                switch (ctx.col) {
                case GFX_COL_BLACK: d |= t; break;
                case GFX_COL_WHITE: d &= ~t; break;
                case GFX_COL_CLEAR:

                        break;
                case GFX_COL_NXOR:
                        d = (d & ~t) | ((d ^ t) & t);
                        break;
                }

                *dp++ = d;
                if (!dm) continue;
                *dm++ |= t;
        }
}

void gfx_px(gfx_context_s ctx, int x, int y)
{
        i_gfx_px_shape(ctx, x, y);
}

void gfx_set_inverted(bool32 inv)
{
#ifdef TARGET_PD
        PD->display->setInverted(inv);
#else
        g_os.inverted = inv;
#endif
}

gfx_context_s gfx_context_create(tex_s dst)
{
        gfx_context_s ctx     = {0};
        ctx.dst               = dst;
        gfx_pattern_s pattern = {
            0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
            0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU};
        ctx.pat = pattern;
        return ctx;
}

static inline u32 i_gfx_pattern_u32_from_u8(u32 b)
{
        return ((b << 24) | (b << 16) | (b << 8) | b);
}

gfx_pattern_s gfx_context_pattern4(int p0, int p1, int p2, int p3)
{
        u32           t0  = i_gfx_pattern_u32_from_u8((p0 << 4) | (p0));
        u32           t1  = i_gfx_pattern_u32_from_u8((p1 << 4) | (p1));
        u32           t2  = i_gfx_pattern_u32_from_u8((p2 << 4) | (p2));
        u32           t3  = i_gfx_pattern_u32_from_u8((p3 << 4) | (p3));
        gfx_pattern_s pat = {t0, t1, t2, t3, t0, t1, t2, t3};
        return pat;
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
void gfx_sprite_tile16(gfx_context_s ctx, v2_i32 pos, rec_i32 rs, int flags)
{
        ASSERT((rs.x & 15) == 0);
        ASSERT((rs.y & 15) == 0);
        ASSERT((rs.w) == 16);
        ASSERT((rs.h) == 16);
        gfx_sprite(ctx, pos, rs, flags);
}

static inline void i_spritepx(u32 *dp, u32 *dm, u32 pt, u32 pp, u32 tt, int m)
{
        u32 t = bswap32(tt & pt); // mask off pattern pixels
        u32 p = bswap32(pp);

        switch (m) {
        case SPRITE_INV: p = ~p; // fallthrough
        case SPRITE_CPY: *dp = (*dp & ~t) | (p & t); break;
        case SPRITE_XOR: p = ~p; // fallthrough
        case SPRITE_NXR: *dp = (*dp & ~t) | ((*dp ^ p) & t); break;
        case SPRITE_W_T: t &= p; // fallthrough
        case SPRITE_B_F: *dp |= t; break;
        case SPRITE_B_T: t &= ~p; // fallthrough
        case SPRITE_W_F: *dp &= ~t; break;
        }

        if (dm) { // write opaque pixel info if mask is present
                *dm |= t;
        }
}

/* fast sprite drawing routine for untransformed sprites
 * blits 32 pixelbits in one loop
 */
void gfx_sprite(gfx_context_s ctx, v2_i32 pos, rec_i32 rs, int flags)
{
        int  xx = flags & SPRITE_FLIP_X ? -1 : +1; // XY flipping factors
        int  yy = flags & SPRITE_FLIP_Y ? -1 : +1;
        int  xa = rs.x + (flags & SPRITE_FLIP_X ? pos.x + rs.w - 1 : -pos.x);
        int  ya = rs.y + (flags & SPRITE_FLIP_Y ? pos.y + rs.h - 1 : -pos.y);
        int  x1 = max_i(pos.x, 0); // pixel bounds on canvas inclusive
        int  y1 = max_i(pos.y, 0);
        int  x2 = min_i(pos.x + rs.w, ctx.dst.w) - 1;
        int  y2 = min_i(pos.y + rs.h, ctx.dst.h) - 1;
        int  b1 = x1 >> 5;                      // first dst byte in x
        int  b2 = x2 >> 5;                      // last dst byte in x
        int  s1 = 31 & (pos.x - rs.x);          // word alignment and shift
        int  s0 = 31 & (32 - s1);               // word alignment and shift
        u32  ul = ((0xFFFFFFFFU >> (x1 & 31))); // boundary masks
        u32  ur = ~(0x7FFFFFFFU >> (x2 & 31));
        u32 *dp = (u32 *)ctx.dst.px;
        u32 *dm = (u32 *)ctx.dst.mk;
        u32 *sp = (u32 *)ctx.src.px;
        u32 *sm = (u32 *)ctx.src.mk;

        // calc pixel coord in source image from canvas pixel coord
        // src_x = (dst_x * xx) + xa   | xx and yy are either +1 or -1
        // dst_x = (src_x - xa) * xx | xx and yy are either +1 or -1

        u32 t; // rendered pixels mask
        u32 p; // black and white bits
        for (int y = y1; y <= y2; y++) {
                u32 pt = ctx.pat.p[y & 7];               // pattern mask
                if (pt == 0) continue;                   // mask is empty
                int ys = (y * yy + ya) * ctx.src.w_word; // source y coord cache

                for (int b = b1; b <= b2; b++) {
                        int xs0 = xa + xx * ((b << 5));
                        int xs1 = xa + xx * ((b << 5) + 31);

                        if (flags & SPRITE_FLIP_X) {
                                // flip x - construct masks manually...
                                // optimize at some point?
                                // xs0 and xs1 are logically swapped
                                xs1 = max_i(xs1, 0);
                                xs0 = min_i(xs0, ctx.src.w - 1);
                                t   = sm ? 0 : 0xFFFFFFFFU;
                                p   = 0;
                                for (int xs = xs1; xs <= xs0; xs++) {
                                        int i0 = (xs >> 5) + ys;
                                        u32 ms = 0x80000000U >> (xs & 31);
                                        if (sm && !(bswap32(sm[i0]) & ms)) continue;
                                        int xd = (xs - xa) * xx;
                                        u32 md = 0x80000000U >> (xd & 31);
                                        t |= md;
                                        if (!(bswap32(sp[i0]) & ms)) continue;
                                        p |= md;
                                }
                        } else {
                                // a destination word overlaps two source words
                                // unless drawing position is word aligned on x
                                xs0    = clamp_i(xs0, 0, ctx.src.w - 1);
                                xs1    = clamp_i(xs1, 0, ctx.src.w - 1);
                                int i0 = (xs0 >> 5) + ys;
                                int i1 = (xs1 >> 5) + ys;

                                p = (bswap32(sp[i0]) << s0) | (bswap32(sp[i1]) >> s1);
                                if (sm)
                                        t = (bswap32(sm[i0]) << s0) | (bswap32(sm[i1]) >> s1);
                                else
                                        t = 0xFFFFFFFFU;
                        }

                        if (b == b1) t &= ul; // mask off out of bounds pixels
                        if (b == b2) t &= ur;
                        if (t == 0) continue;
                        int j = b + y * ctx.dst.w_word;
                        i_spritepx(&dp[j], dm ? &dm[j] : NULL, pt, p, t, ctx.sprmode);
                }
        }
}

sprite_matrix_s sprite_matrix_identity()
{
        sprite_matrix_s m = {1.f, 0.f, 0.f,
                             0.f, 1.f, 0.f,
                             0.f, 0.f, 1.f};
        return m;
}

sprite_matrix_s sprite_matrix_add(sprite_matrix_s a, sprite_matrix_s b)
{
        sprite_matrix_s m;
        for (int n = 0; n < 9; n++)
                m.m[n] = a.m[n] + b.m[n];
        return m;
}

sprite_matrix_s sprite_matrix_sub(sprite_matrix_s a, sprite_matrix_s b)
{
        sprite_matrix_s m;
        for (int n = 0; n < 9; n++)
                m.m[n] = a.m[n] - b.m[n];
        return m;
}

sprite_matrix_s sprite_matrix_mul(sprite_matrix_s a, sprite_matrix_s b)
{
        sprite_matrix_s m;
        for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                        m.m[i + j * 3] = a.m[i + 0 * 3] * b.m[0 + j * 3] +
                                         a.m[i + 1 * 3] * b.m[1 + j * 3] +
                                         a.m[i + 2 * 3] * b.m[2 + j * 3];
                }
        }
        return m;
}

sprite_matrix_s sprite_matrix_rotate(float angle)
{
        float           si = sin_f(angle);
        float           co = cos_f(angle);
        sprite_matrix_s m  = {+co, -si, 0.f,
                              +si, +co, 0.f,
                              0.f, 0.f, 1.f};
        return m;
}

sprite_matrix_s sprite_matrix_scale(float scx, float scy)
{
        sprite_matrix_s m = {scx, 0.f, 0.f,
                             0.f, scy, 0.f,
                             0.f, 0.f, 1.f};
        return m;
}

sprite_matrix_s sprite_matrix_shear(float shx, float shy)
{
        sprite_matrix_s m = {1.f, shx, 0.f,
                             shy, 1.f, 0.f,
                             0.f, 0.f, 1.f};
        return m;
}

sprite_matrix_s sprite_matrix_offset(float x, float y)
{
        sprite_matrix_s m = {1.f, 0.f, x,
                             0.f, 1.f, y,
                             0.f, 0.f, 1.f};
        return m;
}

void gfx_sprite_rotated(gfx_context_s ctx, rec_i32 r, sprite_matrix_s m)
{
        int x1 = 0;
        int y1 = 0;
        int x2 = ctx.dst.w;
        int y2 = ctx.dst.h;
        f32 u1 = (f32)(r.x);
        f32 v1 = (f32)(r.y);
        f32 u2 = (f32)(r.x + r.w);
        f32 v2 = (f32)(r.y + r.h);

        for (int y = y1; y < y2; y++) {
                for (int x = x1; x < x2; x++) {
                        f32 t = (f32)(x + r.x);
                        f32 s = (f32)(y + r.y);
                        f32 u = t * m.m[0] + s * m.m[1] + m.m[2] + 0.5f;
                        f32 v = t * m.m[3] + s * m.m[4] + m.m[5] + 0.5f;
                        if (!(u1 <= u && u < u2)) continue;
                        if (!(v1 <= v && v < v2)) continue;
                        int px, mk;
                        i_gfx_peek(ctx.src, (int)u, (int)v, &px, &mk);
                        if (!mk) continue;
                        ctx.col = px;
                        i_gfx_put_px(ctx, x, y);
                }
        }
}

void gfx_sprite_rotated_(gfx_context_s ctx, v2_i32 pos, rec_i32 r, v2_i32 origin, float angle)
{
        origin.x += r.x;
        origin.y += r.y;
        sprite_matrix_s m1 = sprite_matrix_offset(+(f32)origin.x, +(f32)origin.y);
        sprite_matrix_s m3 = sprite_matrix_offset(-(f32)pos.x - (f32)origin.x, -(f32)pos.y - (f32)origin.y);
        sprite_matrix_s m4 = sprite_matrix_rotate(angle);

        sprite_matrix_s m = sprite_matrix_identity();
        m                 = sprite_matrix_mul(m, m3);
        m                 = sprite_matrix_mul(m, m4);
        m                 = sprite_matrix_mul(m, m1);

        int x1 = 0;
        int y1 = 0;
        int x2 = ctx.dst.w - 1;
        int y2 = ctx.dst.h - 1;
        f32 u1 = (f32)(r.x);
        f32 v1 = (f32)(r.y);
        f32 u2 = (f32)(r.x + r.w);
        f32 v2 = (f32)(r.y + r.h);

        for (int y = y1; y <= y2; y++) {
                for (int x = x1; x <= x2; x++) {
                        f32 t = (f32)(x + r.x);
                        f32 s = (f32)(y + r.y);
                        f32 u = t * m.m[0] + s * m.m[1] + m.m[2] + 0.5f;
                        f32 v = t * m.m[3] + s * m.m[4] + m.m[5] + 0.5f;
                        if (!(u1 <= u && u < u2)) continue;
                        if (!(v1 <= v && v < v2)) continue;
                        int px, mk;
                        i_gfx_peek(ctx.src, (int)u, (int)v, &px, &mk);
                        if (!mk) continue;
                        // ctx.col = px;
                        i_gfx_px_(ctx, x, y, px);
                }
        }
}

void gfx_rec_fill(gfx_context_s ctx, rec_i32 r)
{
        int y1 = MAX(0, r.y);
        int y2 = MIN(ctx.dst.h, r.y + r.h);
        for (int y = y1; y < y2; y++) {
                i_gfx_span_shape(ctx, y, r.x, r.x + r.w);
        }
}

void gfx_tri_fill(gfx_context_s ctx, v2_i32 p0, v2_i32 p1, v2_i32 p2)
{
        v2_i32 t0 = p0;
        v2_i32 t1 = p1;
        v2_i32 t2 = p2;
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
        i32 y1 = MIN(ctx.dst.h - 1, t1.y);
        i32 y2 = MIN(ctx.dst.h - 1, t2.y);
        for (int y = y0; y <= y1; y++) {
                i32 x1 = t0.x + (d0 * (y - t0.y)) / th;
                i32 x2 = t0.x + (d1 * (y - t0.y)) / h1;
                if (x2 < x1) SWAP(i32, x1, x2);
                i_gfx_span_shape(ctx, y, x1, x2);
        }

        for (int y = y1; y <= y2; y++) {
                i32 x1 = t0.x + (d0 * (y - t0.y)) / th;
                i32 x2 = t1.x + (d2 * (y - t1.y)) / h2;
                if (x2 < x1) SWAP(i32, x1, x2);
                i_gfx_span_shape(ctx, y, x1, x2);
        }
}

void gfx_tri(gfx_context_s ctx, v2_i32 p0, v2_i32 p1, v2_i32 p2)
{
        gfx_line(ctx, p0, p1);
        gfx_line(ctx, p0, p2);
        gfx_line(ctx, p1, p2);
}

void gfx_tri_thick(gfx_context_s ctx, v2_i32 p0, v2_i32 p1, v2_i32 p2, int r)
{
        gfx_line_thick(ctx, p0, p1, r);
        gfx_line_thick(ctx, p0, p2, r);
        gfx_line_thick(ctx, p1, p2, r);
}

void gfx_line(gfx_context_s ctx, v2_i32 p0, v2_i32 p1)
{
        int dx = +abs_i(p1.x - p0.x), sx = p0.x < p1.x ? 1 : -1;
        int dy = -abs_i(p1.y - p0.y), sy = p0.y < p1.y ? 1 : -1;
        int er = dx + dy;
        int xi = p0.x;
        int yi = p0.y;
        while (1) {
                i_gfx_px_shape(ctx, xi, yi);
                if (xi == p1.x && yi == p1.y) break;
                int e2 = er * 2;
                if (e2 >= dy) { er += dy, xi += sx; }
                if (e2 <= dx) { er += dx, yi += sy; }
        }
}

// naive thick line, works for now
void gfx_line_thick(gfx_context_s ctx, v2_i32 p0, v2_i32 p1, int r)
{
        int dx = +abs_i(p1.x - p0.x), sx = p0.x < p1.x ? 1 : -1;
        int dy = -abs_i(p1.y - p0.y), sy = p0.y < p1.y ? 1 : -1;
        int er = dx + dy;
        int xi = p0.x;
        int yi = p0.y;
        int r2 = r * r;
        while (1) {
                for (int y = 0; y <= +r; y++) {
                        for (int x = r; x >= 0; x--) {
                                if (x * x + y * y > r2) continue;
                                i_gfx_span_shape(ctx, yi - y, xi - x, xi + x);
                                i_gfx_span_shape(ctx, yi + y, xi - x, xi + x);
                                break;
                        }
                }

                if (xi == p1.x && yi == p1.y) break;
                int e2 = er * 2;
                if (e2 >= dy) { er += dy, xi += sx; }
                if (e2 <= dx) { er += dx, yi += sy; }
        }
}

void gfx_text_ascii(gfx_context_s ctx, fnt_s *font, const char *txt, v2_i32 p)
{
        int len = 1;
        for (int i = 0; txt[i] != '\0'; i++) {
                len++;
        }

        os_spmem_push();
        fntstr_s str = fntstr_create(len, os_spmem_alloc);
        fntstr_append_ascii(&str, txt);
        gfx_text(ctx, font, &str, p);
        os_spmem_pop();
}

void gfx_text(gfx_context_s ctx, fnt_s *font, fntstr_s *str, v2_i32 p)
{
        gfx_text_glyphs(ctx, font, str->chars, str->n, p);
}

void gfx_text_glyphs(gfx_context_s ctx, fnt_s *font, fntchar_s *chars, int l, v2_i32 p)
{
        gfx_context_s cc = ctx;
        cc.src           = font->tex;
        v2_i32 pp        = p;
        for (int i = 0; i < l; i++) {
                fntchar_s c   = chars[i];
                int       cID = (int)c.glyphID;
                int       gy  = cID >> 5; // 32 glyphs in a row
                int       gx  = cID & 31;
                rec_i32   r   = {gx * font->gridw,
                                 gy * font->gridh,
                                 font->gridw,
                                 font->gridh};

                v2_i32 ppp = pp;
                switch (c.effectID) {
                case FNT_EFFECT_SHAKE:
                        ppp.x += rng_range(-1, +1);
                        ppp.y += rng_range(-1, +1);
                        break;
                case FNT_EFFECT_WAVE:
                        ppp.y += sin_q16_fast((os_tick() << 13) + (i << 16)) >> 15;
                        break;
                }

                gfx_sprite(cc, ppp, r, 0);
                pp.x += font->glyph_widths[cID];
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