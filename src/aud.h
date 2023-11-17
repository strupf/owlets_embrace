// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef AUD_H
#define AUD_H

#include "sys/sys.h"

typedef struct {
    sys_wavdata_s wav;
} snd_s;

snd_s snd_load(const char *filename, void *(*allocf)(usize s));
void  snd_play(snd_s s);
void  snd_play_ext(snd_s s, float vol, float pitch);
void  mus_load(const char *filename);
void  mus_stop();

#endif