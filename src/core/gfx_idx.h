// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GFX_IDX_H
#define GFX_IDX_H

#include "pltf/pltf.h"

typedef struct {
    u8 *data; // u8 pixel data
    u16 w;
    u16 h;
} tex_idx_s;

typedef struct {
    tex_idx_s t;
    u16       x;
    u16       y;
    u16       w;
    u16       h;
} tex_idx_rec_s;

typedef struct {
    tex_idx_s dst;
    u16       clip_x1;
    u16       clip_x2;
    u16       clip_y1;
    u16       clip_y2;
} gfx_ctx_idx_s;

#endif