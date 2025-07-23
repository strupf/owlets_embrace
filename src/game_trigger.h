// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_TRIGGER_H
#define GAME_TRIGGER_H

#include "gamedef.h"

enum {
    TRIGGER_DIALOG_NEW_FRAME = 512,
    TRIGGER_DIALOG_END_FRAME = 513,
    TRIGGER_DIALOG_END       = 514,
    TRIGGER_DIALOG           = 515,
    TRIGGER_BATTLEROOM_ENTER = 10000,
    TRIGGER_BATTLEROOM_LEAVE = 10001,
    TRIGGER_CS_UPGRADE       = 11000,
    TRIGGER_COMPANION_FIND   = 9000,
    TRIGGER_BOSS_PLANT       = 10100,
    TRIGGER_CS_INTRO_COMP_1  = 888,
    TRIGGER_CS_FINDING_COMP  = 889,
    TRIGGER_CS_FINDING_HOOK  = 890,
};

void game_on_trigger(g_s *g, i32 trigger);

#endif