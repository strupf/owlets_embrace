// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef FADE_H
#define FADE_H

#include "gamedef.h"

typedef struct {
    int tick;
    int phase;
    int phases;
    int phase_tick[4];

    void *cb_arg;
    void (*callbacks[4])(void *arg);
} fade_s;

int fade_update(fade_s *fade);

#endif