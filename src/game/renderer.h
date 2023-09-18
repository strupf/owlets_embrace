// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef RENDERER_H
#define RENDERER_H

#include "gamedef.h"

enum {
        RENDER_SPRITE,
        RENDER_SPRITE_TRANSFORMED,
        RENDER_REC_FILLED,
        RENDER_REC,
        RENDER_LINE,
        RENDER_LINE_THICK,
        RENDER_TRI,
        RENDER_TRI_FILLED,
        RENDER_CUSTOM,
};

typedef struct {
        rec_i32 src;
        v2_i32  pos;
        tex_s   t;
        int     flags;
} render_thing_sprite_s;

typedef struct {
        int type;
        u32 z; // sort index

} render_thing_s;

#endif