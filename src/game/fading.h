// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef FADING_H
#define FADING_H

typedef struct {
        int ticks;
        int ticks_target;
        void (*cb)(void *arg);
        void *arg;
} timer_s;

void   timer_start(timer_s *t, int ticks, void (*cb)(void *arg), void *arg);
int    timer_lerp(timer_s *t, int from, int to);
bool32 timer_update(timer_s *t);

enum {
        FADE_PHASE_NONE,
        FADE_PHASE_OUT,
        FADE_PHASE_PAUSE,
        FADE_PHASE_IN,
};

typedef struct {
        int phase;
        int ticks;
        int ticks_target; // how long the current phase takes
        int ticks_pause;
        int ticks_in;
        void (*cb_pause)(void *arg);
        void (*cb_finish)(void *arg);
        void *arg;
} fading_s;

// fades out, then calls callback with arg (once it is "black"),
// then pauses the black screen and fades in
void fading_start(fading_s *f,
                  int       ticks_out,
                  int       ticks_pause,
                  int       ticks_in,
                  void (*cb_pause)(void *arg),
                  void (*cb_finish)(void *arg),
                  void *arg);
// only fades over specified number of ticks
void fading_start_half(fading_s *f, int ticks);
int  fading_phase(fading_s *f);
int  fading_update(fading_s *f);
int  fading_lerp(fading_s *f, int from, int to);

#endif