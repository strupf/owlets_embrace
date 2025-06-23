// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GFX_H
#define GFX_H

#include "pltf/pltf.h"

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
    ALIGNAS(16)
    u32 *px; // either black/white words, or black/white and transparent/opaque words interlaced
    i32  w;
    i32  h;
    u16  wword;
    u16  fmt;
} tex_s;

typedef struct { // size: 32 byte on 32-bit CPU (pointer size) = cacheline PD
    ALIGNAS(32)
    tex_s t;
    i32   x;
    i32   y;
    i32   w;
    i32   h;
} texrec_s;

typedef struct gfx_pattern_s { // 32 bytes in size = cacheline on Playdate
    ALIGNAS(16)
    u32 p[4];
} gfx_pattern_s;

typedef struct gfx_ctx_s {
    ALIGNAS(32)
    tex_s         dst;
    i32           clip_x1;
    i32           clip_x2;
    i32           clip_y1;
    i32           clip_y2;
    gfx_pattern_s pat;
} gfx_ctx_s;

typedef struct {
    u8 c1;
    u8 c2;
    u8 space;
} fnt_kerning_s;

typedef struct {
    ALIGNAS(32)
    tex_s          t;
    u8            *widths;
    fnt_kerning_s *kerning;
    u16            n_kerning;
    i16            tracking;
    u8             grid_w;
    u8             grid_h;
} fnt_s;

typedef struct {
    u8 *buf;
    i32 n;
    i32 cap;
} fntstr_s;

typedef struct {
    u32 w;
    u32 h;
    u32 fmt;
} tex_header_s;

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
    //
    SPR_MODE_BLACK_ONLY_WHITE_PT_OPAQUE,
    SPR_MODE_WHITE_ONLY_BLACK_PT_OPAQUE,
};

enum {
    PRIM_MODE_BLACK,       // fills black, pattern holes are transparent
    PRIM_MODE_WHITE,       // fills white, pattern holes are transparent
    PRIM_MODE_WHITE_BLACK, // fills black, pattern holes are white
    PRIM_MODE_BLACK_WHITE, // fills white, pattern holes are black
    PRIM_MODE_INV,         // inverts canvas, pattern holes are transparent
    PRIM_MODE_WHITEN,
    PRIM_MODE_BLACKEN
};

#define GFX_PATTERN_NUM     17
#define GFX_PATTERN_MAX     (GFX_PATTERN_NUM - 1)
#define gfx_pattern_100()   gfx_pattern_bayer_4x4(16)
#define gfx_pattern_75()    gfx_pattern_bayer_4x4(12)
#define gfx_pattern_50()    gfx_pattern_bayer_4x4(8)
#define gfx_pattern_25()    gfx_pattern_bayer_4x4(4)
#define gfx_pattern_0()     gfx_pattern_bayer_4x4(0)
#define gfx_pattern_white() gfx_pattern_100()
#define gfx_pattern_black() gfx_pattern_0()

tex_s    tex_framebuffer();
tex_s    tex_create(i32 w, i32 h, b32 mask, allocator_s a, err32 *err);
texrec_s texrec_from_tex(tex_s t);
i32      tex_px_at(tex_s tex, i32 x, i32 y);
i32      tex_mk_at(tex_s tex, i32 x, i32 y);
void     tex_px(tex_s tex, i32 x, i32 y, i32 col);
void     tex_mk(tex_s tex, i32 x, i32 y, i32 col);
void     tex_px_unsafe(tex_s tex, i32 x, i32 y, i32 col);

static inline void tex_px_unsafe_opaque(tex_s tex, i32 x, i32 y, i32 col)
{
    u32  b = bswap32(0x80000000 >> (x & 31));
    u32 *p = &tex.px[y * tex.wword + (x >> 5)];
    *p     = (col == 0 ? *p & ~b : *p | b);
}

static inline void tex_px_unsafe_opaque_black(tex_s tex, i32 x, i32 y)
{
    u32  b = bswap32(0x80000000 >> (x & 31));
    u32 *p = &tex.px[y * tex.wword + (x >> 5)];
    *p     = *p & ~b;
}

void tex_outline_white(tex_s tex);
void tex_outline_col_small(tex_s tex, i32 col);
void tex_outline_col_ext_small(tex_s tex, i32 col, b32 dia);
void tex_outline_col_ext(tex_s tex, i32 col, b32 dia);

// merges two textures of the same size:
// src: top texture, transparency
// dst: bot texture, opaque
void tex_merge_to_opaque(tex_s dst, tex_s src);

// outlines and merges two textures of the same size:
// src: top texture, transparency
// dst: bot texture, opaque
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
gfx_pattern_s gfx_pattern_bayer_4x4(i32 i);
gfx_pattern_s gfx_pattern_shift(gfx_pattern_s p, i32 x, i32 y);
gfx_pattern_s gfx_pattern_interpolate(i32 num, i32 den);
gfx_pattern_s gfx_pattern_interpolatec(i32 num, i32 den, i32 (*ease)(i32 a, i32 b, i32 num, i32 den));
void          gfx_spr(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode);
void          gfx_spr_copy(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip);
void          gfx_spr_tile_32x32(gfx_ctx_s ctx, texrec_s src, v2_i32 pos);

// tiles spr across screen (true/false for x/y)
void gfx_spr_tileds(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, i32 flip, i32 mode, bool32 tilex, bool32 tiley);
void gfx_spr_tileds_copy(gfx_ctx_s ctx, texrec_s src, v2_i32 pos, bool32 tilex, bool32 tiley);
//
void gfx_rec_fill(gfx_ctx_s ctx, rec_i32 rec, i32 mode);
void gfx_rec_fill_opaque(gfx_ctx_s ctx, rec_i32 rec, i32 mode);
void gfx_rec_strip(gfx_ctx_s ctx, i32 rx, i32 ry, i32 rw, i32 mode);
void gfx_rec_rounded_fill(gfx_ctx_s ctx, rec_i32 rec, i32 r, i32 mode);
void gfx_tri_fill(gfx_ctx_s ctx, tri_i32 t, i32 mode);
void gfx_cir_fill(gfx_ctx_s ctx, v2_i32 p, i32 d, i32 mode);
void gfx_lin(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, i32 mode);
void gfx_lin_thick(gfx_ctx_s ctx, v2_i32 a, v2_i32 b, i32 mode, i32 d);
void gfx_rec(gfx_ctx_s ctx, rec_i32 r, i32 mode);
void gfx_tri(gfx_ctx_s ctx, tri_i32 t, i32 mode);
void gfx_cir(gfx_ctx_s ctx, v2_i32 p, i32 r, i32 mode);
void gfx_fill_rows(tex_s dst, gfx_pattern_s pat, i32 y1, i32 y2);
void gfx_tri_fill_uvw(gfx_ctx_s ctx, v2_i32 tri[3], i32 mode);
void gfx_fill_circle_segment(gfx_ctx_s ctx, v2_i32 p, i32 r,
                             i32 a1, i32 a2, i32 mode);
void gfx_fill_circle_ring_seg(gfx_ctx_s ctx, v2_i32 p, i32 ri, i32 ro,
                              i32 a1_q17, i32 a2_q17, i32 mode);
//
void fnt_draw_str(gfx_ctx_s ctx, fnt_s fnt, v2_i32 pos, const void *s, i32 mode);
void fnt_draw_outline_style(gfx_ctx_s ctx, fnt_s f, v2_i32 pos,
                            const void *str, i32 style, b32 centeredx);
i32  fnt_length_px(fnt_s fnt, const void *txt);
i32  fnt_kerning(fnt_s fnt, i32 c1, i32 c2);

static void spr_blit_p(u32 *dp, u32 sp, u32 sm, u32 pt, i32 mode)
{
    u32 zm = sm & pt;

    switch (mode) {
    case SPR_MODE_INV: sp = ~sp; // fallthrough
    case SPR_MODE_COPY: *dp = (*dp & ~zm) | (sp & zm); break;
    case SPR_MODE_XOR: sp = ~sp; // fallthrough
    case SPR_MODE_NXOR: *dp = (*dp & ~zm) | ((*dp ^ sp) & zm); break;
    case SPR_MODE_WHITE_ONLY: zm &= sp; // fallthrough
    case SPR_MODE_WHITE: *dp |= zm; break;
    case SPR_MODE_BLACK_ONLY: zm &= ~sp; // fallthrough
    case SPR_MODE_BLACK: *dp &= ~zm; break;

    case SPR_MODE_BLACK_ONLY_WHITE_PT_OPAQUE: {
        u32 km = sm & ~sp;
        *dp    = (*dp & ~km) | (km & ~pt);
        break;
    }
    }
}

static void spr_blit_pm(u32 *dp, u32 *dm, u32 sp, u32 pt, u32 sm, i32 mode)
{
    u32 zm = sm & pt;

    switch (mode) {
    case SPR_MODE_INV: sp = ~sp; // fallthrough
    case SPR_MODE_COPY: *dp = (*dp & ~zm) | (sp & zm); break;
    case SPR_MODE_XOR: sp = ~sp; // fallthrough
    case SPR_MODE_NXOR: *dp = (*dp & ~zm) | ((*dp ^ sp) & zm); break;
    case SPR_MODE_WHITE_ONLY: zm &= sp; // fallthrough
    case SPR_MODE_WHITE: *dp |= zm; break;
    case SPR_MODE_BLACK_ONLY: zm &= ~sp; // fallthrough
    case SPR_MODE_BLACK: *dp &= ~zm; break;
    }

    *dm |= zm;
}

static inline void spr_blit_p_copy(u32 *dp, u32 sp, u32 sm)
{
    *dp = (*dp & ~sm) | (sp & sm);
}

#define TEX_STACK(NAME, W, H, MASK)                                      \
    ALIGNAS(8) u32 NAME##_px[(((W) >> 5) << ((MASK) != 0)) * (H)] = {0}; \
                                                                         \
    tex_s NAME = {0};                                                    \
    NAME.w     = W;                                                      \
    NAME.h     = H;                                                      \
    NAME.wword = (((W) >> 5) << ((MASK) != 0));                          \
    NAME.px    = NAME##_px;                                              \
    NAME.fmt   = ((MASK) != 0);

#define TEX_STACK_CTX(NAME, W, H, MASK) \
    TEX_STACK(NAME, W, H, MASK)         \
    gfx_ctx_s NAME##_ctx = gfx_ctx_default(NAME);

enum {
    FNT_GLYPH_NULL   = 0,
    //
    FNT_GLYPH_BTN_DL = 1,
    FNT_GLYPH_BTN_DR,
    FNT_GLYPH_BTN_DU,
    FNT_GLYPH_BTN_DD,
    FNT_GLYPH_BTN_A,
    FNT_GLYPH_BTN_B,
    FNT_GLYPH_BTN_DPAD,
    FNT_GLYPH_BTN_MENU,
    FNT_GLYPH_CRANK,
    // ASCII
    FNT_GLYPH_SPACE = 32,
    // numbers
    FNT_GLYPH_0     = 48,
    FNT_GLYPH_1     = 49,
    FNT_GLYPH_2     = 50,
    FNT_GLYPH_3     = 51,
    FNT_GLYPH_4     = 52,
    FNT_GLYPH_5     = 53,
    FNT_GLYPH_6     = 54,
    FNT_GLYPH_7     = 55,
    FNT_GLYPH_8     = 56,
    FNT_GLYPH_9     = 57,
    // upper case
    FNT_GLYPH_A     = 65,
    FNT_GLYPH_B     = 66,
    FNT_GLYPH_C     = 67,
    FNT_GLYPH_D     = 68,
    FNT_GLYPH_E     = 69,
    FNT_GLYPH_F     = 70,
    FNT_GLYPH_G     = 71,
    FNT_GLYPH_H     = 72,
    FNT_GLYPH_I     = 73,
    FNT_GLYPH_J     = 74,
    FNT_GLYPH_K     = 75,
    FNT_GLYPH_L     = 76,
    FNT_GLYPH_M     = 77,
    FNT_GLYPH_N     = 78,
    FNT_GLYPH_O     = 79,
    FNT_GLYPH_P     = 80,
    FNT_GLYPH_Q     = 81,
    FNT_GLYPH_R     = 82,
    FNT_GLYPH_S     = 83,
    FNT_GLYPH_T     = 84,
    FNT_GLYPH_U     = 85,
    FNT_GLYPH_V     = 86,
    FNT_GLYPH_W     = 87,
    FNT_GLYPH_X     = 88,
    FNT_GLYPH_Y     = 89,
    FNT_GLYPH_Z     = 90,
    // lower case
    FNT_GLYPH_A_L   = 97,
    FNT_GLYPH_B_L   = 98,
    FNT_GLYPH_C_L   = 99,
    FNT_GLYPH_D_L   = 100,
    FNT_GLYPH_E_L   = 101,
    FNT_GLYPH_F_L   = 102,
    FNT_GLYPH_G_L   = 103,
    FNT_GLYPH_H_L   = 104,
    FNT_GLYPH_I_L   = 105,
    FNT_GLYPH_J_L   = 106,
    FNT_GLYPH_K_L   = 107,
    FNT_GLYPH_L_L   = 108,
    FNT_GLYPH_M_L   = 109,
    FNT_GLYPH_N_L   = 110,
    FNT_GLYPH_O_L   = 111,
    FNT_GLYPH_P_L   = 112,
    FNT_GLYPH_Q_L   = 113,
    FNT_GLYPH_R_L   = 114,
    FNT_GLYPH_S_L   = 115,
    FNT_GLYPH_T_L   = 116,
    FNT_GLYPH_U_L   = 117,
    FNT_GLYPH_V_L   = 118,
    FNT_GLYPH_W_L   = 119,
    FNT_GLYPH_X_L   = 120,
    FNT_GLYPH_Y_L   = 121,
    FNT_GLYPH_Z_L   = 122,

};

#endif