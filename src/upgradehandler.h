// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// Handles the screen sized animation when collecting a major upgrade

#ifndef HEROUPGRADE_H
#define HEROUPGRADE_H

#include "gamedef.h"

typedef struct {
    bool32 active;
    int    phase;
    int    t;
} upgradehandler_s;

void   upgradehandler_start_animation(upgradehandler_s *h);
bool32 upgradehandler_in_progress(upgradehandler_s *h);
void   upgradehandler_tick(upgradehandler_s *h);
void   upgradehandler_draw(game_s *g, upgradehandler_s *h, v2_i32 camoffset);
#endif