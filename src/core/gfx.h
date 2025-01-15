// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GFX_H
#define GFX_H

#include "pltf/pltf.h"
#include "util/mem.h"

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
    i32  w;
    i32  h;
} tex_s;

typedef struct {
    tex_s t;
    i32   x;
    i32   y;
    i32   w;
    i32   h;
} texrec_s;

#define GFX_PATTERN_NUM 17
#define GFX_PATTERN_MAX (GFX_PATTERN_NUM - 1)

typedef struct {
    u32 p[8];
} gfx_pattern_s;

typedef struct {
    tex_s         dst;
    gfx_pattern_s pat;
    i32           clip_x1;
    i32           clip_x2;
    i32           clip_y1;
    i32           clip_y2;
} gfx_ctx_s;

typedef struct {
    tex_s t;
    u8   *widths;
    u16   grid_w;
    u16   grid_h;
} fnt_s;

typedef struct {
    u8 *buf;
    i32 n;
    i32 cap;
} fntstr_s;

enum {
    SPR_FLIP_X  = 1, // kBitmapFlippedX
    SPR_FLIP_Y  = 2, // kBitmapFlippedY
    SPR_FLIP_XY = SPR_FLIP_X | SPR_FLIP_Y
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

tex_s         tex_framebuffer();
i32           tex_create_ext(i32 w, i32 h, b32 mask, allocator_s a, tex_s *o_t);
tex_s         tex_create(i32 w, i32 h, alloc_s ma);
tex_s         tex_create_opaque(i32 w, i32 h, alloc_s ma);
tex_s         tex_load(const char *path, alloc_s ma);
i32           tex_px_at(tex_s tex, i32 x, i32 y);
i32           tex_mk_at(tex_s tex, i32 x, i32 y);
void          tex_px(tex_s tex, i32 x, i32 y, i32 col);
void          tex_mk(tex_s tex, i32 x, i32 y, i32 col);
void          tex_outline(tex_s tex, i32 x, i32 y, i32 w, i32 h, i32 col, bool32 dia);
void          tex_outline_white(tex_s tex);
void          tex_merge_to_opaque(tex_s dst, tex_s src);
void          tex_merge_to_opaque_outlined_white(tex_s dst, tex_s src);
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
void          tex_clr(tex_s dst, i32 col);
gfx_pattern_s gfx_pattern_inv(gfx_pattern_s p);
gfx_pattern_s gfx_pattern_2x2(i32 p0, i32 p1);
gfx_pattern_s gfx_pattern_4x4(i32 p0, i32 p1, i32 p2, i32 p3);
gfx_pattern_s gfx_pattern_8x8(i32 p0, i32 p1, i32 p2, i32 p3, i32 p4, i32 p5, i32 p6, i32 p7);
gfx_pattern_s gfx_pattern_bayer_4x4(i32 i);
gfx_pattern_s gfx_pattern_interpolate(i32 num, i32 den);
gfx_pattern_s gfx_pattern_interpolatec(i32 num, i32 den, i32 (*ease)(i32 a, i32 b, i32 num, i32 den));
void          gfx_spr(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode);
void          gfx_spr_tile_32x32(gfx_ctx_s ctx, texrec_s src, v2_i32 pos);
void          gfx_spr_rotated(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, v2_i32 origin, f32 angle);
void          gfx_spr_rotscl(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, v2_i32 origin, f32 angle,
                             f32 sclx, f32 scly);

// tiles spr across screen with tile dimensions tx/ty (pass 0 if not tiled)
void gfx_spr_tiled(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode, i32 tx, i32 ty);

// tiles spr across screen (true/false for x/y)
void gfx_spr_tileds(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode, bool32 x, bool32 y);
//
void gfx_rec_fill(gfx_ctx_s ctx, rec_i32 rec, i32 mode);
void gfx_rec_strip(gfx_ctx_s ctx, i32 rx, i32 ry, i32 rw, i32 mode);
void gfx_rec_rounded_fill(gfx_ctx_s ctx, rec_i32 rec, i32 r, i32 mode);
void gfx_tri_fill(gfx_ctx_s ctx, tri_i32 t, i32 mode);
void gfx_cir_fill(gfx_ctx_s ctx, v2_i32 p, i32 d, i32 mode);
void gfx_lin(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, i32 mode);
void gfx_lin_thick(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, i32 mode, i32 d);
void gfx_rec(gfx_ctx_s ctx, rec_i32 r, i32 mode);
void gfx_tri(gfx_ctx_s ctx, tri_i32 t, i32 mode);
void gfx_cir(gfx_ctx_s ctx, v2_i32 p, i32 r, i32 mode);
void gfx_poly_fill(gfx_ctx_s ctx, v2_i32 *pt, i32 n_pt, i32 mode);
void gfx_fill_rows(tex_s dst, gfx_pattern_s pat, i32 y1, i32 y2);
void gfx_tri_fill_uvw(gfx_ctx_s ctx, v2_i32 tri[3], i32 mode);
void gfx_fill_circle_segment(gfx_ctx_s ctx, v2_i32 p, i32 r,
                             i32 a1, i32 a2, i32 mode);
//
void fnt_draw_ascii(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, const char *text, i32 mode);
void fnt_draw_ascii_mono(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, const char *text, i32 mode, i32 spacing);
i32  fnt_length_px(fnt_s fnt, const char *txt);
i32  fnt_length_px_mono(fnt_s fnt, const char *txt, i32 spacing);

static void spr_blit_p(u32 *dp, u32 sp, u32 sm, i32 mode)
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
}

static void spr_blit_pm(u32 *dp, u32 *dm, u32 sp, u32 sm, i32 mode)
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

    *dm |= sm;
}

#endif