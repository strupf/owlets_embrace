// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "spriteanim.h"

void spriteanim_update(spriteanim_s *a)
{
    if (a->mode == SPRITEANIM_MODE_NONE) return;
    spriteanimdata_s *d = &a->data;
    a->tick++;
    if (a->tick < d->frames[a->frame].ticks) return;

    int f   = a->frame;
    a->tick = 0;
    switch (a->mode) {
    case SPRITEANIM_MODE_LOOP:
        a->frame = (f + 1) % d->n_frames;
        break;
    case SPRITEANIM_MODE_LOOP_REV:
        a->frame = (f - 1 + d->n_frames) % d->n_frames;
        break;
    case SPRITEANIM_MODE_LOOP_PINGPONG:
        break;
    case SPRITEANIM_MODE_ONCE:
        a->frame++;
        if (a->frame == d->n_frames) {
            a->frame = d->n_frames - 1;
            a->mode  = SPRITEANIM_MODE_NONE;
            if (a->cb_finished) {
                a->cb_finished(a, a->cb_arg);
            }
        }
        break;
    case SPRITEANIM_MODE_ONCE_REV:
        a->frame--;
        if (a->frame == -1) {
            a->frame = 0;
            a->mode  = SPRITEANIM_MODE_NONE;
            if (a->cb_finished) {
                a->cb_finished(a, a->cb_arg);
            }
        }
        break;
    }

    if (a->frame != f && a->cb_framechanged) {
        a->cb_framechanged(a, a->cb_arg);
    }
}

texrec_s spriteanim_frame(spriteanim_s *a)
{
    spriteanimdata_s *d = &a->data;
    spriteanimframe_s f = d->frames[a->frame];
    texrec_s          t = {0};
    t.t                 = d->tex;
    t.r.x               = f.x;
    t.r.y               = f.y;
    t.r.w               = d->w;
    t.r.h               = d->h;
    return t;
}