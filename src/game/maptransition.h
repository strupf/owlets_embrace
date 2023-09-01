// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MAPTRANSITION_H
#define MAPTRANSITION_H

#include "gamedef.h"

enum {
        TRANSITION_TICKS       = 15,
        TRANSITION_BLACK_TICKS = 3,
        TRANSITION_FADE_TICKS  = TRANSITION_TICKS - TRANSITION_BLACK_TICKS,
};

enum transition_type {
        TRANSITION_TYPE_SIMPLE,
};

typedef enum {
        TRANSITION_NONE,
        TRANSITION_FADE_OUT,
        TRANSITION_FADE_IN,
} transition_phase_e;

typedef struct {
        transition_phase_e phase;
        int                ticks;
        char               map[64]; // next map to load
        rec_i32            heroprev;
        cam_s              camprev;
        direction_e        enterfrom;
} transition_s;

void game_update_transition(game_s *g);
void game_map_transition_start(game_s *g, const char *filename);

#endif