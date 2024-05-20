// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef AUD_H
#define AUD_H

#include "sys/sys.h"

#define AUD_CLAMP        1
#define MUSCHUNK_MEM     0x1000 // 4 KB
#define MUSCHUNK_SAMPLES (MUSCHUNK_MEM / sizeof(i8))
#define MUS_LEN_FILENAME 64
#define NUM_SNDCHANNEL   8

typedef struct {
    i8 *buf;
    u32 len;
} snd_s;

typedef struct {
    i8 *wavbuf;
    u32 wavlen;
    u32 wavpos;
    i32 vol_q8;
    f32 pitch;
} sndchannel_s;

typedef struct {
    char   filename[MUS_LEN_FILENAME];
    //
    void  *stream;
    i32    datapos;
    i32    streampos; // position in samples
    i32    streamlen;
    i32    chunkpos; // position in samples in chunk
    i32    vol_q8;
    i32    trg_vol_q8;
    bool32 looping;
    //
    i32    fade_out_ticks_og;
    i32    fade_out_ticks;
    i32    fade_in_ticks;
    i32    fade_in_ticks_og;
    //
    alignas(8) i8 chunk[MUSCHUNK_SAMPLES];
} muschannel_s;

typedef struct {
    i32          mus_fade_ticks;
    i32          mus_fade_ticks_max;
    i32          mus_fade_in;
    i32          mus_fade;
    char         mus_new[64];
    muschannel_s muschannel;
    sndchannel_s sndchannel[NUM_SNDCHANNEL];
    bool32       snd_playing_disabled;
    i32          lowpass;
    i32          lowpass_acc;
} AUD_s;

extern AUD_s AUD;

void   aud_init();
void   aud_update();
void   aud_set_lowpass(i32 lp); // 0 for off, otherwise increasing intensity
void   aud_audio(i16 *buf, int len);
void   aud_allow_playing_new_snd(bool32 enabled);
snd_s  snd_load(const char *pathname, alloc_s ma);
void   snd_play(snd_s s, f32 vol, f32 pitch);
void   mus_fade_to(const char *pathname, int ticks_out, int ticks_in);
void   mus_stop();
bool32 mus_play(const char *filename);
bool32 mus_playing();
void   mus_set_vol_q8(i32 vol_q8);
void   mus_set_vol(f32 vol);
void   mus_set_trg_vol(f32 vol);
#endif