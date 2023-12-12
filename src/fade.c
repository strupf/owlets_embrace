// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "fade.h"

int fade_phase(fade_s *f)
{
    return f->phase;
}

int fade_update(fade_s *f)
{
    if (f->phase == FADE_PHASE_NONE) return FADE_PHASE_NONE;

    f->tick++;
    switch (f->phase) {
    case FADE_PHASE_1:
        if (f->tick < f->t1) break;
        f->tick -= f->t1;
        f->phase++;
        if (f->cb_mid)
            f->cb_mid(f->arg);
        // fallthrough
    case FADE_PHASE_2:
        if (f->tick < f->t2) break;
        f->tick -= f->t2;
        f->phase++;
        // fallthrough
    case FADE_PHASE_3:
        if (f->tick < f->t3) break;
        f->tick  = 0;
        f->phase = 0;
        if (f->cb_finished)
            f->cb_finished(f->arg);
    }

    return f->phase;
}

void fade_start(fade_s *f, int t1, int t2, int t3, fade_cb cb_mid, fade_cb cb_finished, void *arg)
{
    f->phase       = FADE_PHASE_1;
    f->tick        = 0;
    f->t1          = t1;
    f->t2          = t2;
    f->t3          = t3;
    f->cb_mid      = cb_mid;
    f->cb_finished = cb_finished;
    f->arg         = arg;
}

int fade_interpolate(fade_s *f, int a, int b)
{
    switch (f->phase) {
    case FADE_PHASE_1: return (a + ((b - a) * f->tick + (f->t1 / 2)) / f->t1);
    case FADE_PHASE_2: return b;
    case FADE_PHASE_3: return (b + ((a - b) * f->tick + (f->t3 / 2)) / f->t3);
    }
    return a;
}