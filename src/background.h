// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include "core/gfx.h"
#include "gamedef.h"

enum {
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

// prerender background for performance mode
void background_init_and_load_from_wad(g_s *g, i32 ID, void *f);
void background_draw(g_s *g, v2_i32 cam_al, v2_i32 cam);

#endif