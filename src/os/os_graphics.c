#include "os_graphics.h"
#include "os_mem.h"

#define OS_GRAPHICS_MEM 0x10000

static struct {
        tex_s dst;
        tex_s tex_tab[NUM_TEXID];

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
        int    w_bytes = (w - 1) / 8 + 1; // 8 pixels in one byte
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
        ASSERT(0); // not implemented
        tex_s t = tex_create(0, 0);
        return t;
}

tex_s tex_get(int ID)
{
        ASSERT(0 <= ID && ID < NUM_TEXID);
        return g_gfx.tex_tab[ID];
}

void gfx_rec_fill(rec_i32 r, int col)
{
}

void gfx_line(int x0, int y0, int x1, int y1)
{
}