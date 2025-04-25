// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef AUD_H
#define AUD_H

#include "pltf/pltf.h"
#include "qoa.h"

#define NUM_SNDCHANNEL    16
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
    void *dat;
    u32   num_samples;
} snd_s;

enum {
    AUD_CMD_SND_PLAY,
    AUD_CMD_SND_MOD,
    AUD_CMD_MUS_PLAY,
    AUD_CMD_MUS_STOP,
    AUD_CMD_MUS_VOL,
    AUD_CMD_MUS_LOOP,
    AUD_CMD_LOWPASS,
};

typedef struct {
    snd_s snd;
    i32   iID;
    u16   vol_q8;
    u16   pitch_q8;
    b8    repeat;
} aud_cmd_snd_play_s;

typedef struct {
    i32 iID;
    i32 stop_ticks;
    i32 vmod_q8;
} aud_cmd_snd_mod_s;

typedef struct {
    u32 hash;
    u32 loop_s1;
    u32 loop_s2;
    u32 ticks_out;
    u32 ticks_in;
    u16 channelID;
    u16 vol_q8;
} aud_cmd_mus_play_s;

typedef struct {
    u32 channelID;
    u32 ticks;
    u32 vol_q8;
} aud_cmd_mus_vol_s;

typedef struct {
    u32 channelID;
    u32 loop_s1;
    u32 loop_s2;
} aud_cmd_mus_loop_s;

typedef struct {
    i32 v;
} aud_cmd_lowpass_s;

typedef struct {
    ALIGNAS(16)
    u32 type;
    union {
        aud_cmd_snd_play_s snd_play;
        aud_cmd_snd_mod_s  snd_mod;
        aud_cmd_mus_play_s mus_play;
        aud_cmd_mus_vol_s  mus_vol;
        aud_cmd_mus_loop_s mus_loop;
        aud_cmd_lowpass_s  lowpass;
    } c;
} aud_cmd_s;

typedef struct sndchannel_s {
    qoa_sfx_s qoa_dat;
    i32       snd_iID;
    i32       stop_v_q8;
    i16       stop_ticks;
    i16       stop_tick;
} sndchannel_s;

enum {
    MUSCHANNEL_FADE_NONE,
    MUSCHANNEL_FADE_OUT,
    MUSCHANNEL_FADE_IN
};

typedef struct muschannel_s {
    qoa_mus_s qoa_str;
    u32       hash_stream;
    u32       hash_queued;
    u16       v_q8;
    u16       v_fade0;
    u16       v_fade1;
    u16       v_fade2;
    u16       fadestate;
    u32       t_fade;
    u32       t_fade_out;
    u32       t_fade_in;
    u32       loop_s1;
    u32       loop_s2;
} muschannel_s;

typedef struct aud_s {
    b16          snd_playing_disabled;
    i16          v_mus_q8;
    i16          v_sfx_q8;
    u32          i_cmd_w_tmp; // write index, copied to i_cmd_w on commit
    u32          i_cmd_w;     // visible to audio thread/context
    u32          i_cmd_r;
    i32          lowpass;
    i32          lowpass_l;
    i32          lowpass_r;
    i32          snd_iID;
    muschannel_s muschannel[NUM_MUSCHANNEL];
    sndchannel_s sndchannel[NUM_SNDCHANNEL];
    aud_cmd_s    cmds[NUM_AUD_CMD_QUEUE];
} aud_s;

err32 aud_init();
void  aud_destroy();
void  aud_audio(aud_s *a, i16 *lbuf, i16 *rbuf, i32 len);
void  aud_allow_playing_new_snd(bool32 enabled);
void  aud_set_lowpass(i32 lp); // 0 for off, otherwise increasing intensity
void  aud_cmd_queue_commit();
void  mus_play(const void *fname);
void  mus_play_extv(const void *fname, u32 s1, u32 s2,
                    i32 t_fade_out, i32 t_fade_in, i32 v_q8);
void  mus_play_ext(i32 channelID, const void *fname, u32 s1, u32 s2,
                   i32 t_fade_out, i32 t_fade_in, i32 v_q8);
void  mus_set_loop(i32 channelID, u32 s1, u32 s2);
void  mus_set_vol_ext(i32 channelID, u16 v_q8, i32 t_fade);
void  mus_set_vol(u16 v_q8, i32 t_fade);
i32   snd_instance_play(snd_s s, f32 vol, f32 pitch);
i32   snd_instance_play_ext(snd_s s, f32 vol, f32 pitch, bool32 repeat);
void  snd_instance_stop(i32 iID);
void  snd_instance_stop_fade(i32 iID, i32 ms, i32 vmod_q8);

#endif