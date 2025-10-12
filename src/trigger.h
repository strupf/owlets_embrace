// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef H_TRIGGER_H
#define H_TRIGGER_H

#include "pltf/pltf.h"

enum {
    TRIGGER_NULL             = 0,
    //
    TRIGGER_CS_INTRO_COMP_1  = 100,
    TRIGGER_CS_FINDING_COMP  = 101,
    TRIGGER_CS_FINDING_HOOK  = 102,
    //
    TRIGGER_BOSS_PLANT       = 500,
    TRIGGER_BATTLEROOM_ENTER = 550,
    TRIGGER_BATTLEROOM_LEAVE = 551,
    //
    TRIGGER_DIA_FRAME_NEW    = 900,
    TRIGGER_DIA_FRAME_END    = 901,
    TRIGGER_DIA_END          = 902,
    TRIGGER_DIA_CHOICE_1     = 903,
    TRIGGER_DIA_CHOICE_2     = 904,
    TRIGGER_DIA_CHOICE_3     = 905,
    TRIGGER_DIA_CHOICE_4     = 906,
};

#endif