// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "fading.h"
#include "os/os.h"

void timer_start(timer_s *t, int ticks, void (*cb)(void *arg), void *arg)
{
        t->ticks        = 0;
        t->ticks_target = ticks;
        t->cb           = cb;
        t->arg          = arg;
}

int timer_lerp(timer_s *t, int from, int to)
{
        return (from + ((to - from) * t->ticks) / t->ticks_target);
}

bool32 timer_update(timer_s *t)
{
        if (t->ticks_target == 0) return 0;
        t->ticks++;
        if (t->ticks >= t->ticks_target) {
                t->ticks_target = 0;
                t->ticks        = 0;
                if (t->cb) {
                        t->cb(t->arg);
                }
                return 0;
        }
        return 1;
}

static void fading_start_(fading_s *f,
                          int       phase,
                          int       ticks_out,
                          int       ticks_pause,
                          int       ticks_in,
                          void (*cb_pause)(void *arg),
                          void (*cb_finish)(void *arg), void *arg)
{
        f->phase        = phase;
        f->ticks        = 0;
        f->ticks_target = ticks_out;
        f->ticks_pause  = ticks_pause;
        f->ticks_in     = ticks_in;
        f->cb_pause     = cb_pause;
        f->cb_finish    = cb_finish;
        f->arg          = arg;
}

void fading_start(fading_s *f,
                  int       ticks_out,
                  int       ticks_pause,
                  int       ticks_in,
                  void (*cb_pause)(void *arg),
                  void (*cb_finish)(void *arg), void *arg)
{
        fading_start_(f, FADE_PHASE_OUT, ticks_out, ticks_pause, ticks_in,
                      cb_pause, cb_finish, arg);
}

void fading_start_half(fading_s *f, int ticks)
{
        fading_start_(f, FADE_PHASE_IN, ticks, 0, 0, NULL, NULL, NULL);
}

int fading_phase(fading_s *f)
{
        return f->phase;
}

int fading_update(fading_s *f)
{
        if (f->phase == 0) return 0;
        f->ticks++;
        if (f->ticks < f->ticks_target) return f->phase;
        f->ticks = 0;
        f->phase++;

        switch (f->phase) {
        case FADE_PHASE_PAUSE: // just entered pause phase
                f->ticks_target = f->ticks_pause;
                if (f->cb_pause) {
                        f->cb_pause(f->arg);
                }
                break;
        case FADE_PHASE_IN:
                f->ticks_target = f->ticks_in;
                break;
        default:
                f->phase = 0;
                if (f->cb_finish) {
                        f->cb_finish(f->arg);
                }
                break;
        }
        return f->phase;
}

int fading_lerp(fading_s *f, int from, int to)
{
        return (from + ((to - from) * f->ticks) / f->ticks_target);
}