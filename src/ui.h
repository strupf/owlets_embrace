// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef UI_H
#define UI_H

#include "gamedef.h"

typedef struct ui_s {
    i32 x;
} ui_s;

void ui_init(g_s *g);
void ui_update(g_s *g);
void ui_draw(g_s *g);

#endif