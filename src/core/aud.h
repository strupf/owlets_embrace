// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef AUD_H
#define AUD_H

#include "sys/sys.h"

#define AUD_CLAMP        1
#define MUSCHUNK_MEM     0x1000 // 4 KB
#define MUSCHUNK_SAMPLES (MUSCHUNK_MEM / sizeof(i16))
#define MUS_LEN_FILENAME 64
#define NUM_SNDCHANNEL   8

typedef struct {
    i16 *buf;
    int  len;
} snd_s;

typedef struct {
    i16 *wavedata;
    int  wavelen;
    int  wavelen_og;
    int  wavepos;
    int  wavepos_inv_q8;
    int  vol_q8;
    int  invpitch_q8; // 1 / pitch
} sndchannel_s;

typedef struct {
    char   filename[MUS_LEN_FILENAME];
    //
    void  *stream;
    int    datapos;
    int    streampos; // position in samples
    int    streamlen;
    int    chunkpos; // position in samples in chunk
    int    vol_q8;
    int    trg_vol_q8;
    bool32 looping;
    //
    int    fade_out_ticks_og;
    int    fade_out_ticks;
    int    fade_in_ticks;
    int    fade_in_ticks_og;
    //
    alignas(8) i16 chunk[MUSCHUNK_SAMPLES];
} muschannel_s;

typedef struct {
    bool32       mute;
    int          mus_fade_ticks;
    int          mus_fade_ticks_max;
    int          mus_fade_in;
    int          mus_fade;
    char         mus_new[64];
    muschannel_s muschannel;
    sndchannel_s sndchannel[NUM_SNDCHANNEL];
    bool32       snd_playing_disabled;
} AUD_s;

extern AUD_s AUD;

void   aud_update();
void   aud_mute(bool32 mute);
void   aud_audio(i16 *buf, int len);
void   aud_allow_playing_new_snd(bool32 enabled);
snd_s  snd_load(const char *pathname, alloc_s ma);
void   snd_play(snd_s s, f32 vol, f32 pitch);
void   mus_fade_to(const char *pathname, int ticks_out, int ticks_in);
void   mus_stop();
bool32 mus_play(const char *filename);
bool32 mus_playing();
void   mus_set_vol(int vol_q8);
void   mus_set_trg_vol(int vol_q8);
#endif