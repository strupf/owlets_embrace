// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include "core/gfx.h"
#include "gamedef.h"

#define BACKGROUND_PT_MAX 64

typedef struct background_s {
    ALIGNAS(16)
    i32 ID;
    i16 offx;
    i16 offy;
    i32 fade_ticks;
    i32 fade_ticks_total;
    i8  fade_pt; // +: white, -: black
    i8  fade_pt_src;
    i8  fade_pt_dst;
    i8  fade_pt_sh_x;
    i8  fade_pt_sh_y;
} background_s;

enum {
    BACKGROUND_ID_NULL,
    BACKGROUND_ID_WHITE,
    BACKGROUND_ID_BLACK,
    BACKGROUND_ID_FOREST_BRIGHT = 8,
    BACKGROUND_ID_FOREST_DARK,
    BACKGROUND_ID_CAVE      = 12,
    BACKGROUND_ID_SNOW      = 16,
    BACKGROUND_ID_WATERFALL = 20,
    BACKGROUND_ID_VERTICAL  = 24,
    BACKGROUND_ID_MOUNTAIN  = 28,
};

void background_set(g_s *g, i32 ID);
void background_draw(g_s *g, v2_i32 cam_al, v2_i32 cam);
void background_fade_to(g_s *g, i32 f_q6, i32 ticks); // f_q6: +64: full white, -64: full black
void background_update(g_s *g);

#endif