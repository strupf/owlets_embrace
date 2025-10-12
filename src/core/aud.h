// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef AUD_H
#define AUD_H

#include "core/qoa.h"
#include "pltf/pltf.h"

enum {
    MUS_CHANNEL_MUSIC,
    MUS_CHANNEL_ENVIRONMENT,
    //
    NUM_MUS_CHANNELS,
};

#define NUM_AUD_CMDS           64
#define NUM_SFX_CHANNELS       16
#define NUM_MUS_CHANNEL_TRACKS 6

#define AUDIO_CTX // macro to tag functions running in the audio context

typedef struct sfx_s {
    void *data;
} sfx_s;

enum {
    AUD_CMD_NULL,
    AUD_CMD_MUS_CUE,
    AUD_CMD_MUS_ADJUST_VOL,
    AUD_CMD_SFX_PLAY,
    AUD_CMD_SFX_MOD,
    AUD_CMD_SFX_STOP,
    AUD_CMD_SFX_STOP_ALL,
    AUD_CMD_SFX_POS_CAM,
    AUD_CMD_SFX_POS,
    AUD_CMD_LOWPASS,
    AUD_CMD_VOL,
    AUD_CMD_STEREO_REQ, // request mono or stereo output
};

// cue new music, or stop music in case of musID == 0, or set volume
typedef struct {
    u16 channel_index;
    u16 musID;
    u16 v_q8;
    u16 millis_fade; // millis to fade out old track if currently playing
} aud_cmd_mus_s;

// play a non-positional sound effect
typedef struct {
    i32   UID;
    void *data;
    u16   v_q8;
    u16   pitch_q8;
} aud_cmd_sfx_s;

// used to stop one (UID != 0) or all sound effects
typedef struct {
    i32 UID;
    u16 millis_fade;
} aud_cmd_sfx_stop_s;

// set position of sound effect with UID; radius is stored in general purpose field of cmd
typedef struct {
    i32 UID;
    i32 px;
    i32 py;
} aud_cmd_sfx_pos_s;

// set position of sound effect with UID; radius is stored in general purpose field of cmd
typedef struct {
    u16 v_q8_mus;
    u16 v_q8_sfx;
} aud_cmd_vol_s;

enum {
    AUD_CMD_FLAG_REPEAT = 1 << 0
};

typedef struct {
    ALIGNAS(16)
    u8  t;
    u8  flags; // to be used freely by any command
    u16 v16;   // to be used freely by any command
    union {    // aligned to 4 on PD
        aud_cmd_mus_s      mus;
        aud_cmd_sfx_s      sfx;
        aud_cmd_sfx_stop_s sfx_stop;
        aud_cmd_sfx_pos_s  sfx_pos;
        aud_cmd_vol_s      vol;
    };
} aud_cmd_s;

#if PLTF_PD_HW
static_assert(sizeof(aud_cmd_s) == 16, "size of cmd");
#endif

// struct to handle volume fading for music, music tracks and sfx
typedef struct aud_vol_handler_s {
    ALIGNAS(8)
    u16 v_q8;
    u16 v_q8_dst; // target volume
    u16 len_acc;  // accumulator for number of samples for fading steps
    u16 len_sub;  // number of samples until volume increment/decrement
} aud_vol_handler_s;

static inline i32 aud_vol_handler_v_q8(aud_vol_handler_s *f)
{
    return (i32)f->v_q8;
}

static inline i32 aud_vol_handler_v_q8_loudness(aud_vol_handler_s *f)
{
    return ((i32)f->v_q8 * (i32)f->v_q8) >> 8; // volume vs. perceived loudness
}

i32  aud_vol_handler_update(aud_vol_handler_s *f, i32 len); // returns new sub len if necessary
void aud_vol_handler_init_dt(aud_vol_handler_s *f, i32 v_q8_dst, i32 l_sub);
void aud_vol_handler_init_millis(aud_vol_handler_s *f, i32 v_q8_dst, i32 millis_fade);

typedef struct mus_channel_track_s {
    qoa_stream_s q;

    // smooths volume just ever so slightly in a hardcoded manner to v_q8_dst
    // to avoid popping noises
    // caller must set v_q8_dst more gradually if actual fades are needed
    u16 v_q8;
    u16 v_q8_dst;
} mus_channel_track_s;

typedef struct mus_channel_s {
    ALIGNAS(32)
    aud_vol_handler_s   vol;
    i32                 musID;
    i32                 musID_queued;
    u32                 v_q8_trg_game; // keep the gameplay's requested volume somewhere
    u32                 seed;          // programming of music if necessary
    u8                  v[8];          // programming of music if necessary
    mus_channel_track_s tracks[NUM_MUS_CHANNEL_TRACKS];
} mus_channel_s;

typedef struct sfx_channel_s {
    ALIGNAS(32)
    qoa_data_s q;

    // cache line
    ALIGNAS(32)
    i32 v_q8;
    i32 v_q8_dst;
    i32 UID; // UID of this sfx
    i32 px;  // position
    i32 py;
    u32 r; // radius [0; 65535]; indicator for being a positional sfx
} sfx_channel_s;

typedef struct aud_s {
    // cache line
    ALIGNAS(32)
    u16 stereo;      // 1 - 2 which way the audio must be output
    u16 stereo_req;  // 1 - 2 does the user request stereo?
    i32 cmd_i_w;     // 4 - 8
    i32 cmd_i_r;     // 4 - 12
    i32 cmd_i_w_tmp; // 4 - 16
    i32 px_cam;      // 4 - 20
    i32 py_cam;      // 4 - 24
    i32 px_cam_dst;  // 4 - 28
    i32 py_cam_dst;  // 4 - 32

    // cache line
    ALIGNAS(32)
    i32 sfx_UID;     // 4 - 4
    b32 sfx_blocked; // 4 - 8
    u16 v_q8_mus;    // 2 - 10 global mus volume; only set via settings
    u16 v_q8_sfx;    // 2 - 12 global sfx volume; only set via settings
    u16 lowpass;     // 2 - 14
    u16 lowpass_dst; // 2 - 16
    i16 lowpass_l;   // 2 - 18
    i16 lowpass_r;   // 2 - 20

    mus_channel_s mus_channels[NUM_MUS_CHANNELS];
    sfx_channel_s sfx_channels[NUM_SFX_CHANNELS];
    aud_cmd_s     cmds[NUM_AUD_CMDS];
} aud_s;

extern aud_s g_AUD;

AUDIO_CTX void aud_audio(i16 *lbuf, i16 *rbuf, i32 len);
aud_cmd_s      aud_cmd_gen(i32 type);
void           aud_cmd_push(aud_cmd_s c);
void           aud_cmd_queue_commit();
void           aud_set_stereo(i32 stereo);
void           aud_lowpass(i32 lp); // 0 for off, otherwise increasing intensity
void           aud_set_vol(i32 v_q8_mus, i32 v_q8_sfx);
i32            aud_calc_positional_vol(aud_s *a, i32 v_q8, i32 px, i32 py, i32 r, i32 zshift, i32 *l_q8, i32 *r_q8);
void           aud_set_pos_cam(i32 px, i32 py, i32 smooth);
//
void           mus_cue(i32 channel_index, i32 musID, i32 millis_fade_out_cur);
void           mus_volume(i32 channel_index, i32 v_q8, i32 millis_fade);
//
i32            sfx_cue(i32 sfxID, i32 v_q8, i32 pitch_q8);
i32            sfx_cuef(i32 sfxID, f32 v, f32 pitch);
i32            sfx_cue_ext(i32 sfxID, i32 v_q8, i32 pitch_q8, bool32 repeat);
i32            sfx_cuef_ext(i32 sfxID, f32 v, f32 pitch, bool32 repeat);
i32            sfx_cue_pos(i32 sfxID, i32 v_q8, i32 pitch_q8, bool32 repeat, i32 px, i32 py, i32 r);
i32            sfx_cuef_pos(i32 sfxID, f32 v, f32 pitch, bool32 repeat, i32 px, i32 py, i32 r);
void           sfx_stop(u32 UID, u32 millis_fade);
void           sfx_stop_all();
void           sfx_block_new(bool32 blocked);
void           sfx_set_pos(i32 UID, i32 px, i32 py, i32 r); // pass r to make a currently playing or queued sfx position; 0 to only update position

#endif