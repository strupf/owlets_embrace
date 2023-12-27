// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef AUD_H
#define AUD_H

#include "sys/sys.h"

typedef sys_wavdata_s aud_snd_s;

#define AUD_WAVES 4

enum {
    WAVE_TYPE_SQUARE,
    WAVE_TYPE_SINE,
    WAVE_TYPE_TRIANGLE,
    WAVE_TYPE_SAW,
};

enum {
    ADSR_NONE,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE,
};

typedef struct {
    int adsr;
    int t;

    i32 vol_sustain;
    i32 vol_peak;
    int attack; // ticks, 44100 = 1s
    int decay;
    int sustain;
    int release;
} envelope_s;

typedef struct {
    int type;

    u32 t; // 0 to 0xFFFFFFFF equals one period
    u32 incr;
    i32 vol;

    envelope_s env;
} wave_s;

typedef struct {
    int  mus_fade_ticks;
    int  mus_fade_ticks_max;
    int  mus_fade_in;
    int  mus_fade;
    char mus_new[64];

    wave_s waves[AUD_WAVES];
} AUD_s;

extern AUD_s AUD;

void      aud_update();
//
aud_snd_s aud_snd_load(const char *pathname, alloc_s ma);
void      aud_snd_play(aud_snd_s s, f32 vol, f32 pitch);
//
void      aud_mus_fade_to(const char *pathname, int ticks_out, int ticks_in);
void      aud_mus_stop();
bool32    aud_mus_playing();

#endif