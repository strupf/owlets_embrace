#ifndef OS_GRAPHICS_H
#define OS_GRAPHICS_H

#include "os_math.h"

enum {
        TEXID_DISPLAY = 0,
        //
        NUM_TEXID
};

typedef struct {
        u8 *px;
        int w_byte;
        int w;
        int h;
} tex_s;

tex_s tex_create(int w, int h);
tex_s tex_load(const char *filename);
tex_s tex_get(int ID);
void  gfx_draw_to(tex_s tex);
void  gfx_rec_fill(rec_i32 r, int col);
void  gfx_line(int x0, int y0, int x1, int y1);

#endif