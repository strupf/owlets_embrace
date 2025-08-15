// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_TRIGGER_H
#define GAME_TRIGGER_H

#include "gamedef.h"

enum {
    TRIGGER_NULL                  = 0,
    //
    TRIGGER_CS_INTRO_COMP_1       = 1000,
    TRIGGER_CS_FINDING_COMP       = 1001,
    TRIGGER_CS_FINDING_HOOK       = 1002,
    //
    TRIGGER_TITLE_PREVIEW_TO_GAME = 2000,
    TRIGGER_BATTLEROOM_ENTER      = 2010,
    TRIGGER_BATTLEROOM_LEAVE      = 2011,
    TRIGGER_BOSS_PLANT            = 2200,
    TRIGGER_DIALOG_NEW_FRAME      = 2500,
    TRIGGER_DIALOG_END_FRAME      = 2501,
    TRIGGER_DIALOG_END            = 2502,
    TRIGGER_DIALOG                = 2503,
};

void game_on_trigger(g_s *g, i32 trigger);

#endif