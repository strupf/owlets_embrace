// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef COINS_H
#define COINS_H

#include "gamedef.h"

#define COINS_MAX 9999

typedef struct {
    i16 n;
    i16 n_change;
    i16 ticks_change;
    u8  fade_q7;
    b8  show_idle; // gets cleared after each frame
    u8  show_idle_tick;
    u8  fade_out_tick;
} coins_s;

void coins_change(g_s *g, i32 n);
void coins_update(g_s *g);
void coins_draw(g_s *g);
i32  coins_total(g_s *g);
void coins_show_idle(g_s *g);

#endif