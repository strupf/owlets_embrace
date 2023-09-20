// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MAPTRANSITION_H
#define MAPTRANSITION_H

#include "cam.h"
#include "game_def.h"

enum {
        TRANSITION_TICKS       = 15,
        TRANSITION_BLACK_TICKS = 3,
        TRANSITION_FADE_TICKS  = TRANSITION_TICKS - TRANSITION_BLACK_TICKS,
};

enum {
        TRANSITION_TYPE_SIMPLE,
};

enum {
        TRANSITION_NONE,
        TRANSITION_FADE_OUT,
        TRANSITION_FADE_IN,
};

struct transition_s {
        int     phase;
        int     ticks;
        char    map[64]; // next map to load
        rec_i32 heroprev;
        cam_s   camprev;
        int     enterfrom;
};

void transition_update(game_s *g);
void transition_start(game_s *g, const char *filename);

#endif