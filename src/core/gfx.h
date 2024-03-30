// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GFX_H
#define GFX_H

#include "sys/sys_types.h"
#include "util/mem.h"

#define GFX_MEM_KB 1024

enum {
    TEX_FMT_OPAQUE, // only color pixels
    TEX_FMT_MASK,   // color and mask interlaced in words
};

enum {
    GFX_COL_BLACK,
    GFX_COL_WHITE,
    GFX_COL_CLEAR,
};

typedef struct {
    u32 *px; // either black/white words, or black/white and transparent/opaque words interlaced
    u16  wword;
    u16  fmt;
    u16  w;
    u16  h;
} tex_s;

typedef struct {
    tex_s   t;
    rec_i32 r;
} texrec_s;

#define GFX_PATTERN_NUM 17
#define GFX_PATTERN_MAX (GFX_PATTERN_NUM - 1)

typedef struct {
    u32 p[8];
} gfx_pattern_s;

typedef struct {
    tex_s         dst;
    gfx_pattern_s pat;
    u16           clip_x1;
    u16           clip_x2;
    u16           clip_y1;
    u16           clip_y2;
} gfx_ctx_s;

typedef struct {
    tex_s t;
    u8   *widths;
    u16   grid_w;
    u16   grid_h;
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

#define gfx_pattern_100()   gfx_pattern_bayer_4x4(16)
#define gfx_pattern_75()    gfx_pattern_bayer_4x4(12)
#define gfx_pattern_50()    gfx_pattern_bayer_4x4(8)
#define gfx_pattern_25()    gfx_pattern_bayer_4x4(4)
#define gfx_pattern_0()     gfx_pattern_bayer_4x4(0)
#define gfx_pattern_white() gfx_pattern_100()
#define gfx_pattern_black() gfx_pattern_0()

fnt_s         fnt_load(const char *filename, alloc_s ma);
tex_s         tex_framebuffer();
tex_s         tex_create(int w, int h, alloc_s ma);
tex_s         tex_create_opaque(int w, int h, alloc_s ma);
tex_s         tex_load(const char *path, alloc_s ma);
int           tex_px_at(tex_s tex, int x, int y);
int           tex_mk_at(tex_s tex, int x, int y);
void          tex_px(tex_s tex, int x, int y, int col);
void          tex_mk(tex_s tex, int x, int y, int col);
void          tex_outline(tex_s tex, int x, int y, int w, int h, int col, bool32 dia);
gfx_ctx_s     gfx_ctx_default(tex_s dst);
gfx_ctx_s     gfx_ctx_display();
gfx_ctx_s     gfx_ctx_unclip(gfx_ctx_s ctx);
gfx_ctx_s     gfx_ctx_clip(gfx_ctx_s ctx, i32 x1, i32 y1, i32 x2, i32 y2);
gfx_ctx_s     gfx_ctx_clip_top(gfx_ctx_s ctx, i32 y1);
gfx_ctx_s     gfx_ctx_clip_bot(gfx_ctx_s ctx, i32 y2);
gfx_ctx_s     gfx_ctx_clip_left(gfx_ctx_s ctx, i32 x1);
gfx_ctx_s     gfx_ctx_clip_right(gfx_ctx_s ctx, i32 x2);
gfx_ctx_s     gfx_ctx_clipr(gfx_ctx_s ctx, rec_i32 r);
gfx_ctx_s     gfx_ctx_clipwh(gfx_ctx_s ctx, i32 x, i32 y, i32 w, i32 h);
void          tex_clr(tex_s dst, int col);
gfx_pattern_s gfx_pattern_2x2(int p0, int p1);
gfx_pattern_s gfx_pattern_4x4(int p0, int p1, int p2, int p3);
gfx_pattern_s gfx_pattern_8x8(int p0, int p1, int p2, int p3, int p4, int p5, int p6, int p7);
gfx_pattern_s gfx_pattern_bayer_4x4(int i);
gfx_pattern_s gfx_pattern_interpolate(int num, int den);
gfx_pattern_s gfx_pattern_interpolatec(int num, int den, int (*ease)(int a, int b, int num, int den));
void          gfx_spr(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, int flip, int mode);
void          gfx_spr_tile(gfx_ctx_s ctx, tex_s tex, int tx, int ty, int ts, v2_i32 pos);
void          gfx_spr_rotated(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, v2_i32 origin, f32 angle);
void          gfx_spr_rotscl(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, v2_i32 origin, f32 angle,
                             f32 sclx, f32 scly);

// tiles spr across screen with tile dimensions tx/ty (pass 0 if not tiled)
void gfx_spr_tiled(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, int flip, int mode, int tx, int ty);

// tiles spr across screen (true/false for x/y)
void gfx_spr_tileds(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, int flip, int mode, bool32 x, bool32 y);
//
#define gfx_rec_fill_display(C, R, M) gfx_rec_fill(C, R, M)
void gfx_rec_fill(gfx_ctx_s ctx, rec_i32 r, int mode);
void gfx_tri_fill(gfx_ctx_s ctx, tri_i32 t, int mode);
void gfx_cir_fill(gfx_ctx_s ctx, v2_i32 p, int d, int mode);
void gfx_lin(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, int mode);
void gfx_lin_thick(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, int mode, int d);
void gfx_rec(gfx_ctx_s ctx, rec_i32 r, int mode);
void gfx_tri(gfx_ctx_s ctx, tri_i32 t, int mode);
void gfx_cir(gfx_ctx_s ctx, v2_i32 p, int r, int mode);
void gfx_poly_fill(gfx_ctx_s ctx, v2_i32 *pt, int n_pt, int mode);
void gfx_fill_rows(tex_s dst, gfx_pattern_s pat, int y1, int y2);
//
void fnt_draw_ascii(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, const char *text, int mode);
void fnt_draw_ascii_mono(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, const char *text, int mode, int spacing);
int  fnt_length_px(fnt_s fnt, const char *txt);
int  fnt_length_px_mono(fnt_s fnt, const char *txt, int spacing);

#endif