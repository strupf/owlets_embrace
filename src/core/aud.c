// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/aud.h"
#include "app.h"
#include "core/qoa.h"
#include "gamedef.h"
#include "util/mathfunc.h"

aud_s g_AUD;

// add definitions for mono and stereo playback functions
#define AUD_FUNC_STEREO 1
AUDIO_CTX void mus_channel_stereo(aud_s *a, mus_channel_s *ch, i16 *lbuf, i16 *rbuf, i32 len);
AUDIO_CTX void sfx_channel_stereo(aud_s *a, sfx_channel_s *ch, i16 *lbuf, i16 *rbuf, i32 len);
#include "core/aud_func.h"
#undef AUD_FUNC_STEREO

#define AUD_FUNC_STEREO 0
AUDIO_CTX void mus_channel_mono(aud_s *a, mus_channel_s *ch, i16 *lbuf, i32 len);
AUDIO_CTX void sfx_channel_mono(aud_s *a, sfx_channel_s *ch, i16 *lbuf, i32 len);
#include "core/aud_func.h"
#undef AUD_FUNC_STEREO

AUDIO_CTX void           mus_channel_reset(mus_channel_s *ch);
AUDIO_CTX sfx_channel_s *sfx_channel_find(aud_s *a, i32 UID);
AUDIO_CTX void           aud_cmd_exe(aud_s *a, aud_cmd_s *c);
AUDIO_CTX void           mus_on_cue(mus_channel_s *ch, i32 musID, i32 v_q8, i32 millis_fade_out_cur);
AUDIO_CTX void           mus_on_tick(mus_channel_s *ch, i32 len);

AUDIO_CTX void aud_cmd_exe(aud_s *a, aud_cmd_s *c)
{
    switch (c->t) {
    case AUD_CMD_STEREO_REQ: {
        a->stereo_req = c->v16;
        break;
    }
    case AUD_CMD_MUS_CUE: {
        aud_cmd_mus_s *cc = &c->mus;
        mus_channel_s *ch = &a->mus_channels[cc->channel_index];
        mus_on_cue(ch, cc->musID, 256, cc->millis_fade);
        break;
    }
    case AUD_CMD_MUS_ADJUST_VOL: {
        aud_cmd_mus_s *cc = &c->mus;
        mus_channel_s *ch = &a->mus_channels[cc->channel_index];
        aud_vol_handler_init_millis(&ch->vol, cc->v_q8, cc->millis_fade);
        break;
    }
    case AUD_CMD_SFX_PLAY: {
        aud_cmd_sfx_s *cc = &c->sfx;
        sfx_channel_s *ch = sfx_channel_find(a, 0);
        if (!ch) break;

        ch->v_q8     = cc->v_q8;
        ch->v_q8_dst = cc->v_q8;
        ch->UID      = cc->UID;
        qoa_data_start(&ch->q, cc->data, cc->pitch_q8, cc->v_q8, c->flags & AUD_CMD_FLAG_REPEAT);
        break;
    }
    case AUD_CMD_SFX_POS: {
        aud_cmd_sfx_pos_s *cc = &c->sfx_pos;
        sfx_channel_s     *ch = sfx_channel_find(a, cc->UID);
        if (!ch) break;

        ch->px = cc->px;
        ch->py = cc->py;
        ch->r  = c->v16;
        break;
    }
    case AUD_CMD_SFX_STOP: {
        aud_cmd_sfx_stop_s *cc = &c->sfx_stop;
        sfx_channel_s      *ch = sfx_channel_find(a, cc->UID);
        if (!ch) break;

        ch->v_q8_dst = 0;
        break;
    }
    case AUD_CMD_SFX_STOP_ALL: {
        aud_cmd_sfx_stop_s *cc = &c->sfx_stop;

        for (i32 n = 0; n < NUM_SFX_CHANNELS; n++) {
            sfx_channel_s *ch = &a->sfx_channels[n];
            ch->v_q8_dst      = 0;
        }
        break;
    }
    case AUD_CMD_LOWPASS: {
        a->lowpass_dst = c->v16;
        if (a->lowpass == 0) { // start lowpass from zero
            a->lowpass_l = 0;
            a->lowpass_r = 0;
        }
        break;
    }
    case AUD_CMD_VOL: {
        aud_cmd_vol_s *cc = &c->vol;
        a->v_q8_mus       = cc->v_q8_mus;
        a->v_q8_sfx       = cc->v_q8_sfx;
        break;
    }
    case AUD_CMD_SFX_POS_CAM: {
        aud_cmd_sfx_pos_s *cc = &c->sfx_pos;
        a->px_cam_dst         = cc->px;
        a->py_cam_dst         = cc->py;
        if (!c->v16) { // don't smooth this position
            a->px_cam = cc->px;
            a->py_cam = cc->py;
        }
        break;
    }
    }
}

AUDIO_CTX void aud_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
    aud_s *a = &g_AUD;

    // execute pending audio commands
    while (a->cmd_i_r != a->cmd_i_w) {
        aud_cmd_exe(a, &a->cmds[a->cmd_i_r]);
        a->cmd_i_r = (a->cmd_i_r + 1) & (NUM_AUD_CMDS - 1);
    }

    if (rbuf) {
        a->stereo = a->stereo_req;
    } else { // forced mono output
        a->stereo = 0;
    }

    if (rbuf) {
        for (i32 n = 0; n < NUM_MUS_CHANNELS; n++) {
            mus_channel_s *ch = &a->mus_channels[n];
            mus_channel_stereo(a, ch, lbuf, rbuf, len);
        }
        for (i32 n = 0; n < NUM_SFX_CHANNELS; n++) {
            sfx_channel_s *ch = &a->sfx_channels[n];
            sfx_channel_stereo(a, ch, lbuf, rbuf, len);
        }
    } else {
        for (i32 n = 0; n < NUM_MUS_CHANNELS; n++) {
            mus_channel_s *ch = &a->mus_channels[n];
            mus_channel_mono(a, ch, lbuf, len);
        }
        for (i32 n = 0; n < NUM_SFX_CHANNELS; n++) {
            sfx_channel_s *ch = &a->sfx_channels[n];
            sfx_channel_mono(a, ch, lbuf, len);
        }
    }
}

aud_cmd_s aud_cmd_gen(i32 type)
{
    aud_s    *a = &g_AUD;
    aud_cmd_s c = {0};
    c.t         = type;
    return c;
}

void aud_cmd_push(aud_cmd_s c)
{
    aud_s *a = &g_AUD;
    i32    i = (a->cmd_i_w_tmp + 1) & (NUM_AUD_CMDS - 1);

    if (i == a->cmd_i_r) { // full?
        // pltf_log("audio queue full!\n");
    } else {
        assert(a->cmd_i_w_tmp < ARRLEN(a->cmds));
        a->cmds[a->cmd_i_w_tmp] = c;
        a->cmd_i_w_tmp          = i;
    }
}

void aud_cmd_queue_commit()
{
#if PLTF_PD_HW
    // data memory barrier; prevent memory access reordering
    // ensures all commands are fully written before making them visible
    // to the audio context via the write index
    // -> needed because of interrupts
    __asm volatile("dmb");
#endif
#if PLTF_SDL
    pltf_sdl_audio_lock();
#endif
    aud_s *a   = &g_AUD;
    a->cmd_i_w = a->cmd_i_w_tmp;
#if PLTF_SDL
    pltf_sdl_audio_unlock();
#endif
}

void aud_set_pos_cam(i32 px, i32 py, i32 smooth)
{
    aud_cmd_s          cmd = aud_cmd_gen(AUD_CMD_SFX_POS_CAM);
    aud_cmd_sfx_pos_s *cc  = &cmd.sfx_pos;
    cmd.v16                = smooth;
    cc->px                 = px;
    cc->py                 = py;
    aud_cmd_push(cmd);
}

void aud_set_stereo(i32 stereo)
{
    aud_cmd_s cmd = aud_cmd_gen(AUD_CMD_STEREO_REQ);
    cmd.v16       = stereo;
    aud_cmd_push(cmd);
}

void aud_lowpass(i32 lp)
{
    aud_cmd_s cmd = aud_cmd_gen(AUD_CMD_LOWPASS);
    cmd.v16       = lp;
    aud_cmd_push(cmd);
}

void aud_set_vol(i32 v_q8_mus, i32 v_q8_sfx)
{
    aud_cmd_s      cmd = aud_cmd_gen(AUD_CMD_VOL);
    aud_cmd_vol_s *cc  = &cmd.vol;
    cc->v_q8_mus       = clamp_i32(v_q8_mus, 0, 256);
    cc->v_q8_sfx       = clamp_i32(v_q8_sfx, 0, 256);
    aud_cmd_push(cmd);
}

// returns distance based mono volume i.e. is audible
i32 aud_calc_positional_vol(aud_s *a, i32 v_q8, i32 px, i32 py, i32 r, i32 zshift, i32 *l_q8, i32 *r_q8)
{
    assert(l_q8 && r_q8);
    if (!r) { // not positional
        *l_q8 = v_q8;
        *r_q8 = v_q8;
        return v_q8;
    }

    i32 d = r - distance_appr_i32(px, py, a->px_cam, a->py_cam);
    if (d <= 0) { // outside hearing range
        *l_q8 = 0;
        *r_q8 = 0;
        return 0;
    }

    // volume falloff: quad
    i32 v_q8_distance = (i32)(((u32)d * (u32)d) / (((u32)r * (u32)r) >> 8)); // [0; 256] (d * d) / ((r * r) >> 8)
    i32 v_q8_pos      = (v_q8 * v_q8_distance + 255) >> 8;                   // [0; 256]

    if (a->stereo) {
        // stereo output, distance based volume and left/right panning
        // -128 | 0 | +128
        // rshift X -> scaling factor, kinda representing distance of camera from gameplay
        // have to fine tune because aggressive panning is unpleasant
        i32 pan = clamp_i32((128 * (px - a->px_cam)) >> zshift, -128, +128); // [-128; +128]
        i32 vl  = 128 + pan;                                                 // [0; 256]
        i32 vr  = 128 - pan;                                                 // [0; 256]
        *l_q8   = v_q8_pos - ((v_q8_pos * vl * vl + 65535) >> 16);           // [0; 256]
        *r_q8   = v_q8_pos - ((v_q8_pos * vr * vr + 65535) >> 16);           // [0; 256]
    } else {
        *l_q8 = v_q8_pos; // [0; 256]
        *r_q8 = v_q8_pos; // [0; 256]
    }
    return v_q8;
}

void aud_vol_handler_init_dt(aud_vol_handler_s *f, i32 v_q8_dst, i32 l_sub)
{
    if (l_sub < 12) { // it doesn't make sense to have tiny fading steps
        f->v_q8    = v_q8_dst;
        f->len_sub = 0;
        f->len_acc = 0;
    } else {
        f->len_sub = l_sub;
        f->len_acc = l_sub;
    }

    f->v_q8_dst = v_q8_dst;
}

void aud_vol_handler_init_millis(aud_vol_handler_s *f, i32 v_q8_dst, i32 millis_fade)
{
    // steps in number of samples until changing volume by +-4:
    // bigger volume steps result in less sub division of the audio playback loop
    // s = (millis * 44100) / ((dt_vol * 1000) / 4) -> / 4 because of steps of 4

    i32 d = v_q8_dst - (i32)f->v_q8;
    if (d) {
        i32 num = millis_fade * 441;
        i32 den = (abs_i32(d) * 10) / 4;
        aud_vol_handler_init_dt(f, v_q8_dst, num / den);
    }
}

i32 aud_vol_handler_update(aud_vol_handler_s *f, i32 len)
{
    i32 d     = (i32)f->v_q8_dst - (i32)f->v_q8;
    i32 l_sub = len;

    if (d) {
        l_sub = min_i32(len, f->len_acc);
        assert(0 < l_sub);
        f->len_acc -= l_sub;
        if (f->len_acc == 0) {
            f->v_q8 += clamp_sym_i32(sgn_i32(d) * 4, d);
            f->len_acc = f->len_sub;
        }
    }
    return l_sub;
}

// hardcoded test for algorithmic music
#if 0
 switch (am->musID) {
    case 0: return;
    case MUSID_ENCOUNTER: {
        am->c0 += len;
        if (249900 <= am->c0) { // 5666 milliseconds blocks
            am->c0                            = 0;
            am->c1                            = 1 - am->c1;
            static const char melodies[4][32] = {"M_ENCOUNTER_M1",
                                                 "M_ENCOUNTER_M2",
                                                 "M_ENCOUNTER_M3",
                                                 "M_ENCOUNTER_M4"};
            static const char rhythms[4][32]  = {"M_ENCOUNTER_RA",
                                                 "M_ENCOUNTER_RB",
                                                 "M_ENCOUNTER_RC",
                                                 "M_ENCOUNTER_RD"};
            static const char strings[2][32]  = {"M_ENCOUNTER_S0",
                                                 "M_ENCOUNTER_S1"};

            qoa_stream_s *q0 = aud_mus_play_str(am, am->c1 * 3, melodies[rngr_i32(0, 3)]);
            q0->repeat       = 0;
            qoa_stream_s *q1 = aud_mus_play_str(am, am->c1 * 3 + 1, rhythms[rngr_i32(0, 3)]);
            q1->repeat       = 0;
            qoa_stream_s *q2 = aud_mus_play_str(am, am->c1 * 3 + 2, strings[rngr_i32(0, 1)]);
            q2->repeat       = 0;
        }
        break;
    }
    default: break;
    }



  switch (am->musID) {
        default: break;
        case MUSID_INTRO: {
            qoa_stream_s *q        = aud_mus_play_str(am, 0, "M_INTRO");
            q->loop_sample_pos_beg = 325679;
            break;
        }
        case MUSID_WATERFALL: {
            qoa_stream_s *q        = aud_mus_play_str(am, 0, "M_WATERFALL");
            q->loop_sample_pos_beg = 226822;
            break;
        }
        case MUSID_CAVE: {
            qoa_stream_s *q        = aud_mus_play_str(am, 0, "M_CAVE");
            q->loop_sample_pos_beg = 498083;
            break;
        }
        case MUSID_ANCIENT_TREE: {
            qoa_stream_s *q        = aud_mus_play_str(am, 0, "M_ANCIENT_TREE");
            q->loop_sample_pos_beg = 564480;
            break;
        }
        case MUSID_SNOW: {
            qoa_stream_s *q        = aud_mus_play_str(am, 0, "M_SNOW");
            q->loop_sample_pos_beg = 368146;
            break;
        }
        case MUSID_FOREST: {
            qoa_stream_s *q        = aud_mus_play_str(am, 0, "M_FOREST");
            q->loop_sample_pos_beg = 769768;
            break;
        }
        case MUSID_TEST: {
            qoa_stream_s *q0 = aud_mus_play_str(am, 0, "M_ENCOUNTER_M1");
            break;
        }
        case MUSID_ENCOUNTER: {
            qoa_stream_s *q0 = aud_mus_play_str(am, 0, "M_ENCOUNTER_M1");
            q0->repeat       = 0;
            qoa_stream_s *q1 = aud_mus_play_str(am, 1, "M_ENCOUNTER_RA");
            q1->repeat       = 0;
            qoa_stream_s *q2 = aud_mus_play_str(am, 2, "M_ENCOUNTER_S1");
            q2->repeat       = 0;
            break;
        }
        }
#endif