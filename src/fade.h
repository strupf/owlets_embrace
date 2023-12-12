// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef FADE_H
#define FADE_H

// used for fading things in and out, generally
// e.g.: phase 1 fade out, phase 2 black screen, phase 2 fade in
// a, b: times when callbacks are called
//
// |
// |   ________
// |  /|      |\    
// | / |      | \
// |/__|______|__\___
//   1 a  2    3  b

enum {
    FADE_PHASE_NONE,
    FADE_PHASE_1,
    FADE_PHASE_2,
    FADE_PHASE_3,
};

typedef void (*fade_cb)(void *arg);

typedef struct {
    int tick;
    int phase;
    int t1;
    int t2;
    int t3;

    void   *arg;
    fade_cb cb_mid;
    fade_cb cb_finished;
} fade_s;

int  fade_phase(fade_s *f);
int  fade_update(fade_s *f); // returns current phase
void fade_start(fade_s *f, int t1, int t2, int t3, fade_cb cb_mid, fade_cb cb_finished, void *arg);
int  fade_interpolate(fade_s *f, int a, int b);

#endif