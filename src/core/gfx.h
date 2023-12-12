// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GFX_H
#define GFX_H

#include "sys/sys_types.h"
#include "util/mem.h"

#define GFX_MEM_KB 1024

typedef struct {
    u8 *px;
    u8 *mk;
    int w;
    int h;
    int wword;
    int wbyte;
} tex_s;

typedef struct {
    tex_s   t;
    rec_i32 r;
} texrec_s;

typedef struct {
    u32 p[8];
} gfx_pattern_s;

typedef struct {
    tex_s         dst;
    gfx_pattern_s pat;
    u8           *st; // stencil
} gfx_ctx_s;

typedef struct {
    tex_s t;
    u8   *widths;
    int   grid_w;
    int   grid_h;
} fnt_s;

typedef struct {
    u8 *buf;
    int n;
    int cap;
} fntstr_s;

enum {
    SPR_FLIP_X = 1, // kBitmapFlippedX
    SPR_FLIP_Y = 2, // kBitmapFlippedY
};

enum {                   // pattern holes always transparent
    SPR_MODE_COPY,       // kDrawModeCopy
    SPR_MODE_WHITE_ONLY, // kDrawModeBlackTransparent
    SPR_MODE_BLACK_ONLY, // kDrawModeWhiteTransparent
    SPR_MODE_BLACK,      // kDrawModeFillBlack
    SPR_MODE_WHITE,      // kDrawModeFillWhite
    SPR_MODE_NXOR,       // kDrawModeNXOR
    SPR_MODE_XOR,        // kDrawModeXOR
    SPR_MODE_INV,        // kDrawModeInverted
};

enum {
    PRIM_MODE_BLACK,       // fills black, pattern holes are transparent
    PRIM_MODE_WHITE,       // fills white, pattern holes are transparent
    PRIM_MODE_WHITE_BLACK, // fills black, pattern holes are white
    PRIM_MODE_BLACK_WHITE, // fills white, pattern holes are black
    PRIM_MODE_INV,         // inverts canvas, pattern holes are transparent
};

enum {
    TEX_CLR_WHITE,
    TEX_CLR_BLACK,
    TEX_CLR_TRANSPARENT,
};

fnt_s         fnt_load(const char *filename, void *(allocf)(usize s));
tex_s         tex_framebuffer();
tex_s         tex_create(int w, int h, void *(*allocf)(usize s));
tex_s         tex_load(const char *path, void *(*allocf)(usize s));
int           tex_px_at(tex_s tex, int x, int y);
int           tex_mk_at(tex_s tex, int x, int y);
void          tex_px(tex_s tex, int x, int y, int col);
void          tex_mk(tex_s tex, int x, int y, int col);
void          tex_outline(tex_s tex, int x, int y, int w, int h, int col, bool32 dia);
gfx_ctx_s     gfx_ctx_default(tex_s dst);
gfx_ctx_s     gfx_ctx_stencil(tex_s dst, tex_s stc);
void          tex_clr(tex_s dst, int col);
gfx_pattern_s gfx_pattern_4x4(int p0, int p1, int p2, int p3);
gfx_pattern_s gfx_pattern_8x8(int p0, int p1, int p2, int p3, int p4, int p5, int p6, int p7);
gfx_pattern_s gfx_pattern_bayer_4x4(int i);
gfx_pattern_s gfx_pattern_interpolate(int num, int den);
gfx_pattern_s gfx_pattern_interpolate_hor_stripes(int num, int den);
//
void          gfx_spr(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, int flip, int mode);
void          gfx_spr_rotated(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, v2_i32 origin, f32 angle);
//
void          gfx_rec_fill(gfx_ctx_s ctx, rec_i32 r, int mode);
void          gfx_tri_fill(gfx_ctx_s ctx, tri_i32 t, int mode);
void          gfx_cir_fill(gfx_ctx_s ctx, v2_i32 p, int r, int mode);
void          gfx_lin(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, int mode);
void          gfx_lin_thick(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, int mode, int r);
void          gfx_rec(gfx_ctx_s ctx, rec_i32 r, int mode);
void          gfx_tri(gfx_ctx_s ctx, tri_i32 t, int mode);
void          gfx_cir(gfx_ctx_s ctx, v2_i32 p, int r, int mode);
void          gfx_textri(gfx_ctx_s ctx, tex_s src, tri_i32 tri, tri_i32 tex, int mode);
//
void          fnt_draw_ascii(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, const char *text, int mode);

#endif