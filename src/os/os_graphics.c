#include "os_internal.h"

enum {
        OS_GRAPHICS_MEM = 0x10000
};

static struct {
        tex_s dst;
        tex_s tex_tab[NUM_TEXID];
        fnt_s fnt_tab[NUM_FNTID];

        memarena_s      mem;
        ALIGNAS(4) char mem_raw[OS_GRAPHICS_MEM];
} g_gfx;

void os_graphics_init(tex_s framebuffer)
{
        g_gfx.tex_tab[TEXID_DISPLAY] = framebuffer;
        memarena_init(&g_gfx.mem, g_gfx.mem_raw, OS_GRAPHICS_MEM);
}

tex_s tex_create(int w, int h)
{
        // bytes per row - rows aligned to 32 bit
        int    w_bytes = sizeof(int) * ((w - 1) / 32 + 1);
        size_t s       = sizeof(u8) * w_bytes * h;
        u8    *pxmem   = (u8 *)memarena_allocz(&g_gfx.mem, s);
        tex_s  t       = {pxmem,
                          w_bytes,
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
        for (int y = 0; y < h; y++) {
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
        g_gfx.tex_tab[ID] = t;
}

tex_s tex_get(int ID)
{
        ASSERT(0 <= ID && ID < NUM_TEXID);
        return g_gfx.tex_tab[ID];
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
        g_gfx.fnt_tab[ID] = f;
}

fnt_s fnt_get(int ID)
{
        ASSERT(0 <= ID && ID < NUM_FNTID);
        return g_gfx.fnt_tab[ID];
}

void fntstr_append_glyph(fntstr_s *f, int glyphID)
{
        ASSERT(f->n < f->c);
        fntchar_s c      = {glyphID, 0};
        f->chars[f->n++] = c;
}

void fntstr_append_ascii(fntstr_s *f, const char *txt)
{
        for (int i = 0; txt[i] != '\0'; i++) {
                int c = (int)txt[i];
                fntstr_append_glyph(f, c);
        }
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

        int idx = (x >> 3) + y * t.w_byte;
        int i   = t.px[idx];
        return ((i & (1u << (7 - (x & 7)))) > 0);
}

static inline void i_gfx_put_px(tex_s t, int x, int y, int col, int mode)
{
        if (!(0 <= x && x < t.w && 0 <= y && y < t.h)) return;

        int idx = (x >> 3) + y * t.w_byte;
        int bit = (x & 7);
        int i   = t.px[idx];
        if (col == 1) {
                i |= (1u << (7 - bit)); // set bit
        } else {
                i &= ~(1u << (7 - bit)); // clear bit
        }
        t.px[idx] = (u8)i;
}

void gfx_draw_to(tex_s tex)
{
        g_gfx.dst = tex;
}

void gfx_sprite(tex_s src, v2_i32 pos, rec_i32 rs, int flags)
{
        int xx, yy, xy, yx;
        int ff = flags & 7;
        int xa = rs.x;
        int ya = rs.y;
        switch (ff) {
        case 0: xx = +1, yy = +1, xy = yx = 0; break;
        case 4: xy = +1, yx = +1, xx = yy = 0; break;                                 // rotate 90 cw, then flip x
        case 1: xx = -1, yy = +1, xy = yx = 0, xa += rs.w - 1; break;                 // flipx
        case 2: xx = +1, yy = -1, xy = yx = 0, ya += rs.h - 1; break;                 // flipy
        case 5: xy = +1, yx = -1, xx = yy = 0, ya += rs.h - 1; break;                 // rotate 90 cw
        case 6: xy = -1, yx = +1, xx = yy = 0, xa += rs.w - 1; break;                 // rotate 90 ccw
        case 3: xx = -1, yy = -1, xy = yx = 0, xa += rs.w - 1, ya += rs.h - 1; break; // flip xy / rotate 180
        case 7: xy = -1, yx = -1, xx = yy = 0, xa += rs.w - 1, ya += rs.h - 1; break; // rotate 90 cw, then flip y
        }

        tex_s dst = g_gfx.dst;
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
                        int px = i_gfx_peek_px(src, xs, ys);
                        i_gfx_put_px(dst, xp, yp, px, 0);
                }
        }
}

void gfx_rec_fill(rec_i32 r, int col)
{
        tex_s   dst = g_gfx.dst;
        rec_i32 ri;
        rec_i32 rd = {0, 0, dst.w, dst.h};
        if (!intersect_rec(rd, r, &ri)) return;

        int x1 = ri.x;
        int y1 = ri.y;
        int x2 = ri.x + ri.w;
        int y2 = ri.y + ri.h;
        for (int y = y1; y < y2; y++) {
                for (int x = x1; x < x2; x++) {
                        i_gfx_put_px(dst, x, y, col, 0);
                }
        }
}

void gfx_line(int x0, int y0, int x1, int y1, int col)
{
        tex_s dst = g_gfx.dst;
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
                        er += dy;
                        xi += sx;
                }
                if (e2 <= dx) {
                        er += dx;
                        yi += sy;
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
                rec_i32   r   = {gx * font->gridw, gy * font->gridh,
                                 font->gridw, font->gridh};
                gfx_sprite(fonttex, p, r, 0);
                p.x += font->glyph_widths[cID];
        }
}
