// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "fade.h"

int fade_update(fade_s *f)
{
    if (f->phase == 0) return 0;

    f->tick++;

    if (f->tick < f->phase_tick[f->phase]) return f->phase;

    f->tick = 0;

    if (f->callbacks[f->phase]) {
        f->callbacks[f->phase](f->cb_arg);
    }

    f->phase++;
    if (f->phase >= f->phases) {
        f->phase = 0;
    }
    return f->phase;
}