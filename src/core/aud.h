// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef AUD_H
#define AUD_H

#include "pltf/pltf.h"

#define LEN_MUS_NAME      24
#define NUM_SNDCHANNEL    12
#define NUM_AUD_CMD_QUEUE 128

#if 1
#define AUD_MUS_ASSERT assert
#define AUD_MUS_DEBUG
#else
#define AUD_MUS_ASSERT(X)
#endif

enum {
    AUD_MUSCHANNEL_0_LAYER_0,
    AUD_MUSCHANNEL_0_LAYER_1,
    AUD_MUSCHANNEL_0_LAYER_2,
    AUD_MUSCHANNEL_0_LAYER_3,
    AUD_MUSCHANNEL_1,
    AUD_MUSCHANNEL_2,
    //
    NUM_MUSCHANNEL
};

typedef struct snd_s {
    u8 *buf;
    u32 len;
} snd_s;

enum {
    AUD_CMD_SND_PLAY,
    AUD_CMD_SND_MODIFY,
    AUD_CMD_MUS_PLAY,
    AUD_CMD_LOWPASS,
};

// new background music has priority over sfx, lowpass etc.
#define AUD_CMD_PRIORITY_MUS_PLAY 1

typedef struct {
    snd_s snd;
    u32   iID;
    u16   vol_q8;
    u16   pitch_q8;
} aud_cmd_snd_play_s;

typedef struct {
    u32    iID;
    bool16 stop;
    u16    vol_q8;
} aud_cmd_snd_modify_s;

typedef struct {
    char mus_name[LEN_MUS_NAME];
    u8   channelID;
    u8   vol_q8;
    u8   ticks_out;
    u8   ticks_in;
} aud_cmd_mus_play_s;

static_assert(sizeof(aud_cmd_mus_play_s) <= 28, "Music command size");

typedef struct {
    i32 v;
} aud_cmd_lowpass_s;

typedef struct {
    alignas(32) // cache line on Cortex M7
        u16 type;
    u16     priority; // for dropping unimportant commands when the queue is full
    union {
        aud_cmd_snd_play_s   snd_play;
        aud_cmd_snd_modify_s snd_modify;
        aud_cmd_mus_play_s   mus_play;
        aud_cmd_lowpass_s    lowpass;
    } c;
} aud_cmd_s;

typedef struct adpcm_s {
    u8 *data;        // pointer to sample data (2x 4 bit samples per byte)
    u32 len;         // number of samples
    u32 len_pitched; // length of pitched buffer
    u32 data_pos;    // index in data array
    u32 pos;         // current sample index in original buffer
    u32 pos_pitched; // current sample index in pitched buffer
    u16 pitch_q8;
    u16 ipitch_q8;
    i16 hist;
    i16 step_size;
    u8  nibble;
    u8  curr_byte;   // current byte value of the sample
    i16 curr_sample; // current decoded i16 sample value
    i16 vol_q8;      // playback volume in Q8
} adpcm_s;

typedef struct sndchannel_s {
    u32     snd_iID;
    adpcm_s adpcm;
} sndchannel_s;

typedef struct muschannel_s {
    void   *stream;
    u32     total_bytes_file;
    adpcm_s adpcm;
    u8      chunk[256];
    char    mus_name[LEN_MUS_NAME];
    i32     trg_vol_q8;
    bool32  looping;
} muschannel_s;

typedef struct AUD_s {
    muschannel_s muschannel[NUM_MUSCHANNEL];
    sndchannel_s sndchannel[NUM_SNDCHANNEL];
    u32          i_cmd_w_tmp; // write index, copied to i_cmd_w on commit
    u32          i_cmd_w;     // visible to audio thread/context
    u32          i_cmd_r;
    aud_cmd_s    cmds[NUM_AUD_CMD_QUEUE];
    u32          snd_iID; // unique snd instance ID counter
    bool32       snd_playing_disabled;
    i32          lowpass;
    i32          lowpass_acc;
} AUD_s;

extern AUD_s AUD;

void  aud_audio(i16 *lbuf, i16 *rbuf, i32 len);
void  aud_allow_playing_new_snd(bool32 enabled);
void  aud_set_lowpass(i32 lp); // 0 for off, otherwise increasing intensity
void  aud_cmd_queue_commit();
snd_s snd_load(const char *pathname, alloc_s ma);
u32   snd_instance_play(snd_s s, f32 vol, f32 pitch); // returns an integer to refer to an active sound instance
void  snd_instance_stop(u32 snd_iID);
void  snd_instance_set_vol(u32 snd_iID, f32 vol);
void  mus_play(const char *fname);

#endif