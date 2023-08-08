// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os_internal.h"

#if defined(TARGET_DESKTOP) // =================================================
void os_backend_graphics_init()
{
        InitWindow(400 * OS_DESKTOP_SCALE, 240 * OS_DESKTOP_SCALE, "raylib");
        Image img = GenImageColor(416, 240, BLACK);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        g_os.tex = LoadTextureFromImage(img);
        SetTextureFilter(g_os.tex, TEXTURE_FILTER_POINT);
        UnloadImage(img);
        SetTargetFPS(60);
}

void os_backend_graphics_close()
{
        UnloadTexture(g_os.tex);
        CloseWindow();
}

void os_backend_graphics_begin()
{
        os_memclr4(g_os.framebuffer, sizeof(g_os.framebuffer));
}

void os_backend_graphics_end()
{
        static const Color t_rgb[2] = {0x31, 0x2F, 0x28, 0xFF,
                                       0xB1, 0xAF, 0xA8, 0xFF};

        for (int y = 0; y < 240; y++) {
                for (int x = 0; x < 400; x++) {
                        int i         = (x >> 3) + y * 52;
                        int k         = x + y * 416;
                        int byt       = g_os.framebuffer[i];
                        int bit       = (byt & (0x80 >> (x & 7))) > 0;
                        g_os.texpx[k] = t_rgb[g_os.inverted ? !bit : bit];
                }
        }
        UpdateTexture(g_os.tex, g_os.texpx);
}

void os_backend_graphics_flip()
{
        static const Rectangle rsrc =
            {0, 0, 416, 240};
        static const Rectangle rdst =
            {0, 0, 416 * OS_DESKTOP_SCALE, 240 * OS_DESKTOP_SCALE};
        static const Vector2 vorg =
            {0, 0};
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(g_os.tex, rsrc, rdst, vorg, 0.f, WHITE);
        EndDrawing();
}
#endif

#if defined(TARGET_PD) // ======================================================
static void (*PD_display)(void);
static void (*PD_drawFPS)(int x, int y);
static void (*PD_markUpdatedRows)(int start, int end);

void os_backend_graphics_init()
{
        PD_display         = PD->graphics->display;
        PD_drawFPS         = PD->system->drawFPS;
        PD_markUpdatedRows = PD->graphics->markUpdatedRows;
        PD->display->setRefreshRate(0.f);
        g_os.framebuffer = PD->graphics->getFrame();
}

void os_backend_graphics_close()
{
}

void os_backend_graphics_begin()
{
        os_memclr4(g_os.framebuffer, OS_FRAMEBUFFER_SIZE);
}

void os_backend_graphics_end()
{
        PD_markUpdatedRows(0, LCD_ROWS - 1); // mark all rows as updated
        PD_drawFPS(384, 0);
        PD_display(); // update all rows
}

void os_backend_graphics_flip()
{
}
#endif // ======================================================================

tex_s tex_create(int w, int h)
{
        // bytes per row - rows aligned to 32 bit
        int    w_word = (w - 1) / 32 + 1;
        int    w_byte = w_word * 4;
        size_t s      = sizeof(u8) * w_byte * h;

        // twice the size (1 bit white/black, 1 bit transparent/opaque)
        void *mem = memarena_allocz(&g_os.assetmem, s * 2);
        tex_s t   = {(u8 *)mem,
                     (u8 *)((char *)mem + s),
                     w_word,
                     w_byte,
                     w,
                     h};
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
        tex_s t = tex_create(w, h);
        for (int y = 0; y < h * 2; y++) {
                for (int x = 0; x < t.w_byte; x++) {
                        int c1 = (char_hex_to_int(*c++)) << 4;
                        int c2 = (char_hex_to_int(*c++));

                        t.px[x + y * t.w_byte] = (u8)(c1 | c2);
                }
        }
        os_spmem_pop();
        return t;
}

void tex_put(int ID, tex_s t)
{
        ASSERT(0 <= ID && ID < NUM_TEXID);
        g_os.tex_tab[ID] = t;
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
        return ((i & (1 << (7 - (x & 7)))) > 0);
}

static inline void i_gfx_peek(tex_s t, int x, int y, int *px, int *op)
{
        if (!(0 <= x && x < t.w && 0 <= y && y < t.h)) {
                *px = 0;
                *op = 0;
                return;
        }
        int i = (x >> 3) + y * t.w_byte;
        int m = 1 << (7 - (x & 7));
        *px   = ((t.px[i] & m) > 0);
        *op   = (t.mask[i] & m);
}

static inline void i_gfx_put_px(tex_s t, int x, int y, int col, int mode)
{
        if (!(0 <= x && x < t.w && 0 <= y && y < t.h)) return;

        int i = (x >> 3) + y * t.w_byte;
        int s = 1 << (7 - (x & 7));
        if (col)
                t.px[i] |= s; // set bit
        else
                t.px[i] &= ~s; // clear bit
        if (t.mask) t.mask[i] |= s;
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

/* this is a naive image drawing routine
 * can flip an image x, y and diagonal (rotate 90 degree)
 * could be hugely improved by figuring out a way to put multiple
 * pixels at once
 */
void gfx_sprite(tex_s src, v2_i32 pos, rec_i32 rs, int flags)
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

/* fast sprite drawing routine for untransformed sprites
 * blits 32 pixelbits in one loop
 */
void gfx_sprite_fast(tex_s src, v2_i32 pos, rec_i32 rs)
{
        tex_s dst = g_os.dst;
        int   zx  = dst.w - pos.x;
        int   zy  = dst.h - pos.y;
        int   x1  = (0 >= -pos.x ? 0 : -pos.x) + rs.x;
        int   y1  = (0 >= -pos.y ? 0 : -pos.y) + rs.y;
        int   x2  = (rs.w <= zx ? rs.w : zx) + rs.x - 1;
        int   y2  = (rs.h <= zy ? rs.h : zy) + rs.y - 1;

        int cc           = pos.x - rs.x;
        int sh1          = (32 - (cc & 31)) & 31; // relative word alignment
        int sh0          = 32 - sh1;
        int b1           = x1 >> 5;
        int b2           = x2 >> 5;
        u32 *restrict dp = (u32 *)dst.px;
        u32 *restrict dm = (u32 *)dst.mask;
        for (int y = y1; y <= y2; y++) {
                int yd            = (y + pos.y - rs.y) * dst.w_word;
                int ii            = b1 + y * src.w_word;
                u32 *restrict sm_ = &((u32 *)src.mask)[ii];
                u32 *restrict sp_ = &((u32 *)src.px)[ii];
                for (int b = b1; b <= b2; b++) {
                        int u  = (b == b1 ? x1 & 31 : 0);
                        int v  = (b == b2 ? x2 & 31 : 31);
                        u32 m  = (0xFFFFFFFFu >> u) & ~(0x7FFFFFFFu >> v);
                        u32 sm = endian_u32(*sm_++) & m;
                        u32 sp = endian_u32(*sp_++);
                        u32 t0 = sm >> sh0;
                        u32 p0 = sp >> sh0;
                        u32 t1 = sm << sh1;
                        u32 p1 = sp << sh1;
                        int j0 = (((b << 5) + cc) >> 5) + yd;
                        int j1 = (((b << 5) + cc + 31) >> 5) + yd;
                        u32 d0 = endian_u32(dp[j0]);
                        u32 d1 = endian_u32(dp[j1]);
                        dp[j0] = endian_u32((d0 & ~t0) | (p0 & t0));
                        dp[j1] = endian_u32((d1 & ~t1) | (p1 & t1));
                        if (!dm) continue;
                        dm[j0] |= endian_u32(endian_u32(dm[j0]) | t0);
                        dm[j1] |= endian_u32(endian_u32(dm[j1]) | t1);
                }
        }
}

void gfx_rec_fill(rec_i32 r, int col)
{
        tex_s   dst = g_os.dst;
        rec_i32 ri;
        rec_i32 rd = {0, 0, dst.w, dst.h};
        if (!intersect_rec(rd, r, &ri)) return;
        int zx = dst.w - ri.x;
        int zy = dst.h - ri.y;
        int x1 = (0 >= -ri.x ? 0 : -ri.x);
        int y1 = (0 >= -ri.y ? 0 : -ri.y);
        int x2 = (ri.w <= zx ? ri.w : zx) - 1;
        int y2 = (ri.h <= zy ? ri.h : zy) - 1;

        // relative word alignment
        int sh1          = (32 - (ri.x & 31)) & 31; // bit offset of words
        int sh0          = 32 - sh1;
        int b1           = x1 >> 5;
        int b2           = x2 >> 5;
        u32 *restrict dp = (u32 *)dst.px;
        u32 *restrict dm = (u32 *)dst.mask;
        for (int y = y1; y <= y2; y++) {
                int yd = (y + ri.y) * dst.w_word;
                for (int b = b1; b <= b2; b++) {
                        int u  = (b == b1 ? x1 & 31 : 0);
                        int v  = (b == b2 ? x2 & 31 : 31);
                        u32 sm = (0xFFFFFFFFu >> u) & ~(0x7FFFFFFFu >> v);
                        u32 sp = col ? 0xFFFFFFFFu : 0;
                        u32 t0 = sm >> sh0;
                        u32 p0 = sp >> sh0;
                        u32 t1 = sm << sh1;
                        u32 p1 = sp << sh1;
                        int j0 = (((b << 5) + ri.x) >> 5) + yd;
                        int j1 = (((b << 5) + ri.x + 31) >> 5) + yd;
                        u32 d0 = endian_u32(dp[j0]);
                        u32 d1 = endian_u32(dp[j1]);
                        dp[j0] = endian_u32((d0 & ~t0) | (p0 & t0));
                        dp[j1] = endian_u32((d1 & ~t1) | (p1 & t1));
                        if (!dm) continue;
                        dm[j0] = endian_u32(endian_u32(dm[j0]) | t0);
                        dm[j1] = endian_u32(endian_u32(dm[j1]) | t1);
                }
        }
}

void gfx_line(int x0, int y0, int x1, int y1, int col)
{
        tex_s dst = g_os.dst;
        int   dx  = +ABS(x1 - x0);
        int   dy  = -ABS(y1 - y0);
        int   sx  = x0 < x1 ? 1 : -1;
        int   sy  = y0 < y1 ? 1 : -1;
        int   er  = dx + dy;
        int   xi  = x0;
        int   yi  = y0;
        while (1) {
                i_gfx_put_px(dst, xi, yi, col, 0);
                if (xi == x1 && yi == y1) return;
                int e2 = er * 2;
                if (e2 >= dy) {
                        er += dy, xi += sx;
                }
                if (e2 <= dx) {
                        er += dx, yi += sy;
                }
        }
}

// naive thick line, works for now
void gfx_line_thick(int x0, int y0, int x1, int y1, int r, int col)
{
        tex_s dst = g_os.dst;
        int   dx  = +ABS(x1 - x0);
        int   dy  = -ABS(y1 - y0);
        int   sx  = x0 < x1 ? 1 : -1;
        int   sy  = y0 < y1 ? 1 : -1;
        int   er  = dx + dy;
        int   xi  = x0;
        int   yi  = y0;
        int   r2  = r * r;
        while (1) {
                for (int y = -r; y <= +r; y++) {
                        for (int x = -r; x <= +r; x++) {
                                if (x * x + y * y > r2) continue;
                                i_gfx_put_px(dst, xi + x, yi + y, col, 0);
                        }
                }

                if (xi == x1 && yi == y1) return;
                int e2 = er * 2;
                if (e2 >= dy) {
                        er += dy, xi += sx;
                }
                if (e2 <= dx) {
                        er += dx, yi += sy;
                }
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

                gfx_sprite(fonttex, p, r, 0);
                p.x += font->glyph_widths[cID];
        }
}

void gfx_text_glyphs(fnt_s *font, fntchar_s *chars, int l, int x, int y)
{
        static u32 rng     = 213;
        tex_s      fonttex = font->tex;
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
                        pp.x += rng_i16(&rng) % 2;
                        pp.y += rng_i16(&rng) % 2;
                        break;
                case FNT_EFFECT_WAVE:
                        pp.y += sin_q16((os_tick() << 13) + (i << 16)) >> 15;
                        break;
                }

                gfx_sprite(fonttex, pp, r, 0);
                p.x += font->glyph_widths[cID];
        }
}
