// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "fade.h"

int fade_update(fade_s *f)
{
    if (f->phase != FADE_NONE) return FADE_NONE;

    f->tick++;

    switch (f->phase) {
    case FADE_PHASE_I:
        if (f->tick >= f->ticks_i) {
            f->tick = 0;
            f->phase++;
            if (f->cb_on_w) {
                f->cb_on_w(f->cb_arg);
            }
        }
        break;
    case FADE_PHASE_W:
        if (f->tick >= f->ticks_w) {
            f->tick = 0;
            f->phase++;
        }
        break;
    case FADE_PHASE_O:
        if (f->tick >= f->ticks_o) {
            f->tick  = 0;
            f->phase = FADE_NONE;
            if (f->cb_on_o) {
                f->cb_on_o(f->cb_arg);
            }
        }
        break;
    }
    return f->phase;
}