// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "aud.h"
#include "sys/sys.h"

snd_s snd_load(const char *filename, void *(*allocf)(usize s))
{
    snd_s s;
    s.wav = sys_load_wavdata(filename, allocf);
    return s;
}

void snd_play(snd_s s)
{
    snd_play_ext(s, 1.f, 1.f);
}

void snd_play_ext(snd_s s, float vol, float pitch)
{
    sys_wavdata_play(s.wav, vol, pitch);
}

void mus_load(const char *filename)
{
    sys_mus_play(filename);
}

void mus_stop()
{
    sys_mus_stop();
}