// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef FADE_H
#define FADE_H

#include "gamedef.h"

enum {
    FADE_NONE,
    FADE_PHASE_I,
    FADE_PHASE_W,
    FADE_PHASE_O,
};

typedef struct {
    int tick;
    int phase;

    int ticks_i;
    int ticks_w;
    int ticks_o;

    void *cb_arg;
    void (*cb_on_w)(void *arg);
    void (*cb_on_o)(void *arg);
} fade_s;

int fade_update(fade_s *fade);

#endif