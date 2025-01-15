// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef AUD_H
#define AUD_H

#include "pltf/pltf.h"
#include "qoa.h"

#define NUM_SNDCHANNEL    12
#define NUM_AUD_CMD_QUEUE 64

#if 0
#define AUD_MUS_ASSERT assert
#define AUD_MUS_DEBUG
#else
#define AUD_MUS_ASSERT(X)
#endif

enum {
    AUD_MUSCHANNEL_0_LAYER_0,
    AUD_MUSCHANNEL_0_LAYER_1,
    //
    NUM_MUSCHANNEL
};

typedef struct snd_s {
    u32   num_samples;
    void *dat;
} snd_s;

enum {
    AUD_CMD_SND_PLAY,
    AUD_CMD_SND_MODIFY,
    AUD_CMD_MUS_PLAY,
    AUD_CMD_MUS_STOP,
    AUD_CMD_MUS_MODIFY,
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
    u32 hash;
    u8  channelID;
    u8  vol_q8;
    u8  ticks_out;
    u8  ticks_in;
} aud_cmd_mus_play_s;

typedef struct {
    i32 v;
} aud_cmd_lowpass_s;

typedef struct {
    ALIGNAS(32) // cache line on Cortex M7
    u16 type;
    u16 priority; // for dropping unimportant commands when the queue is full
    union {
        aud_cmd_snd_play_s   snd_play;
        aud_cmd_snd_modify_s snd_modify;
        aud_cmd_mus_play_s   mus_play;
        aud_cmd_lowpass_s    lowpass;
    } c;
} aud_cmd_s;

typedef struct sndchannel_s {
    u32       snd_iID;
    qoa_sfx_s qoa_dat;
} sndchannel_s;

typedef struct muschannel_s {
    qoa_mus_s qoa_str;
    i32       vol_from;
    i32       vol_to;
    i32       ticks;
    i32       ticks_total;
} muschannel_s;

typedef struct aud_s {
    u32          i_cmd_w_tmp; // write index, copied to i_cmd_w on commit
    u32          i_cmd_w;     // visible to audio thread/context
    u32          i_cmd_r;
    u32          snd_iID; // unique snd instance ID counter
    bool32       snd_playing_disabled;
    i32          lowpass;
    i32          lowpass_acc;
    muschannel_s muschannel[NUM_MUSCHANNEL];
    sndchannel_s sndchannel[NUM_SNDCHANNEL];
    aud_cmd_s    cmds[NUM_AUD_CMD_QUEUE];
} aud_s;

i32   aud_init();
void  aud_destroy();
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