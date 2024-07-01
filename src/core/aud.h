// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef AUD_H
#define AUD_H

#include "pltf/pltf.h"

#define AUD_CLAMP        1
#define MUS_LEN_FILENAME 64
#define NUM_MUSCHANNEL   2
#define NUM_SNDCHANNEL   8

typedef struct snd_s {
    u8 *buf;
    u32 len;
} snd_s;

typedef struct adpcm_s {
    u8 *data;        // pointer to sample data (2x 4 bit samples per byte)
    u32 len;         // number of samples
    i32 vol_q8;      // playback volume in Q8
    u32 pos;         // current sample index in original buffer
    u32 pos_pitched; // current sample index in pitched buffer
    u32 len_pitched; // length of pitched buffer
    u32 data_pos;    // index in data array
    i32 hist;
    i32 step_size;
    u32 nibble;
    u32 curr_byte;   // current byte value of the sample
    i32 curr_sample; // current decoded i16 sample value
    u32 pitch_q8;
    u32 ipitch_q8;
} adpcm_s;

typedef struct sndchannel_s {
    u8     *data;
    adpcm_s adpcm;
} sndchannel_s;

typedef struct muschannel_s {
    void   *stream;
    adpcm_s adpcm;
    u8      chunk[512];
    i32     trg_vol_q8;
    bool32  looping;
    i32     fade_ticks;
    i32     fade_ticks_max;
    i32     fade_in;
    i32     fade;
    char    filename_new[64];
    char    filename[MUS_LEN_FILENAME];
} muschannel_s;

typedef struct {
    muschannel_s muschannel;
    sndchannel_s sndchannel[NUM_SNDCHANNEL];
    bool32       snd_playing_disabled;
    i32          lowpass;
    i32          lowpass_acc;
    f32          snd_pitch;
} AUD_s;

extern AUD_s AUD;

void   aud_init();
void   aud_update();
void   aud_set_lowpass(i32 lp); // 0 for off, otherwise increasing intensity
void   aud_set_global_pitch(f32 pitch);
void   aud_audio(i16 *lbuf, i16 *rbuf, i32 len);
void   aud_allow_playing_new_snd(bool32 enabled);
void   mus_fade_to(const char *pathname, i32 ticks_out, i32 ticks_in);
void   mus_stop();
bool32 mus_play(const char *filename);
bool32 mus_playing();
void   mus_set_vol_q8(i32 vol_q8);
void   mus_set_vol(f32 vol);
void   mus_set_trg_vol(f32 vol);
snd_s  snd_load(const char *pathname, alloc_s ma);
void   snd_play(snd_s s, f32 vol, f32 pitch);
#endif