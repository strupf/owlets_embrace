// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "ui.h"
#include "game.h"

enum {
    UI_HEALTH,
    UI_COINS
};

void ui_init(g_s *g);
void ui_update(g_s *g);
void ui_draw(g_s *g);
void ui_hide_all(g_s *g);
void ui_show_all(g_s *g);