// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GFX_1BIT_H
#define GFX_1BIT_H

#include "pltf/pltf.h"

// this is a successor to the tex structure to be used in
// future revisions of the gfx functions
enum {
    TEX_1BIT_FMT_OPAQUE, // only color pixels
    TEX_1BIT_FMT_MASK,   // color and mask interlaced in words
};

typedef struct { // masked bitmap
    u32 p;       // black/white
    u32 m;       // opaque/transparent
} tex_1bit_pxmk_s;

typedef struct { // opaque bitmap
    u32 p;       // black/white
} tex_1bit_px_s;

typedef struct {
    void *data;
    u16   fmt;
    u16   wword;
    u16   w;
    u16   h;
} tex_1bit_s;

typedef struct {
    tex_1bit_s t;
    u16        x;
    u16        y;
    u16        w;
    u16        h;
} tex_1bit_rec_s;

typedef struct {
    tex_1bit_s dst;
    u16        clip_x1;
    u16        clip_x2;
    u16        clip_y1;
    u16        clip_y2;
    u32        pt[8];
} gfx_ctx_1bit_s;

#include "gfx.h"
void gfx_new_tpl(gfx_ctx_s ctx, texrec_s trec, v2_i32 psrc, i32 sy, i32 mode);

#endif