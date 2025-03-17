// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "aud.h"
#include "app.h"
#include "gamedef.h"
#include "qoa.h"
#include "util/mathfunc.h"

static void       aud_cmd_execute(aud_cmd_s cmd_u);
static void       aud_push_cmd(aud_cmd_s c);
static inline u32 aud_cmd_next_index(u32 i);
//
static void       muschannel_stop(muschannel_s *mc);
static void       sndchannel_stop(sndchannel_s *ch);

// no callback yet - can't be interruped
err32 aud_init()
{
    aud_s *aud    = &APP->aud;
    aud->v_mus_q8 = ((i32)SETTINGS.vol_mus << 8) / SETTINGS_VOL_MAX;
    aud->v_sfx_q8 = ((i32)SETTINGS.vol_sfx << 8) / SETTINGS_VOL_MAX;
    return 0;
}

// removed from callback - can't be interruped anymore
void aud_destroy()
{
    for (i32 n = 0; n < NUM_MUSCHANNEL; n++) {
        qoa_mus_end(&APP->aud.muschannel[n].qoa_str);
    }
}

void aud_audio(aud_s *aud, i16 *lbuf, i16 *rbuf, i32 len)
{
    // assumption: provided buffers are 0 filled

    // flush audio commands
    // catch up the read index to the write index in the
    // circular command buffer
    while (aud->i_cmd_r != aud->i_cmd_w) {
        aud_cmd_s cmd_u = aud->cmds[aud->i_cmd_r];
        aud->i_cmd_r    = aud_cmd_next_index(aud->i_cmd_r);
        aud_cmd_execute(cmd_u);
    }

    for (i32 n = 0; n < NUM_MUSCHANNEL; n++) {
        muschannel_s *ch = &aud->muschannel[n];
        if (ch->ticks_total) {
            i32 v = 0;

            ch->ticks += len;
            if (ch->ticks_total <= ch->ticks) {
                v               = ch->vol_to;
                ch->ticks_total = 0;
            } else {
                v = lerp_i32(ch->vol_from, ch->vol_to,
                             ch->ticks, ch->ticks_total);
            }
            qoa_mus_set_vol(&ch->qoa_str, v);
        }

        if (qoa_mus_active(&ch->qoa_str)) {
            qoa_mus(&ch->qoa_str, lbuf, rbuf, len, aud->v_mus_q8);
        }
    }

    for (i32 n = 0; n < NUM_SNDCHANNEL; n++) {
        sndchannel_s *ch = &aud->sndchannel[n];
        if (qoa_sfx_active(&ch->qoa_dat)) {
            qoa_sfx_play(&ch->qoa_dat, lbuf, rbuf, len, aud->v_sfx_q8);
        }
    }

    if (aud->lowpass) {
        i16 *b = lbuf;
        for (i32 n = 0; n < len; n++) {
            aud->lowpass_acc += ((i32)*b - aud->lowpass_acc) >> aud->lowpass;
            *b++ = (i16)aud->lowpass_acc;
        }
    }
    // mcpy(rbuf, lbuf, len * sizeof(i16));

#if 0
#define REVERB_SAMPLES 32768
    static i16 revbuf[REVERB_SAMPLES];
    static i32 revp;

    for (i32 i = 0; i < len; i++) {
        lbuf[i] += revbuf[revp] >> 1;
        revbuf[revp] = lbuf[i];
        revp         = (revp + 1) % REVERB_SAMPLES;
    }
#endif
}

static void aud_cmd_execute(aud_cmd_s cmd_u)
{
    switch (cmd_u.type) {
    default: break;
    case AUD_CMD_SND_PLAY: {
        aud_cmd_snd_play_s *c = &cmd_u.c.snd_play;

        for (i32 i = 0; i < NUM_SNDCHANNEL; i++) {
            sndchannel_s *ch = &APP->aud.sndchannel[i];
            qoa_sfx_s    *q  = &ch->qoa_dat;
            if (qoa_sfx_active(q)) continue;

            qoa_sfx_start(q, c->snd.num_samples, c->snd.dat, c->pitch_q8, c->vol_q8, 0);
            break;
        }
        break;
    }
    case AUD_CMD_MUS_PLAY: {
        aud_cmd_mus_play_s *c  = &cmd_u.c.mus_play;
        muschannel_s       *mc = &APP->aud.muschannel[0];
        qoa_mus_s          *q  = &mc->qoa_str;
        if (qoa_mus_active(q)) {
            qoa_mus_end(q);
        }

        void     *f;
        wad_el_s *wad_el;
        if (wad_open(c->hash, &f, &wad_el)) {
            qoa_mus_start(q, f);
        }
        break;
    }
    case AUD_CMD_LOWPASS: {
        aud_cmd_lowpass_s *c = &cmd_u.c.lowpass;
        APP->aud.lowpass     = c->v;
        break;
    }
    }
}

void aud_allow_playing_new_snd(bool32 enabled)
{
    APP->aud.snd_playing_disabled = !enabled;
}

// Called by gameplay thread/context
static void aud_push_cmd(aud_cmd_s c)
{
    // temporary write index
    // peek new position and see if the queue is full
    u32 i = aud_cmd_next_index(APP->aud.i_cmd_w_tmp);
    pltf_audio_lock();
    bool32 is_full = (i == APP->aud.i_cmd_r);
    pltf_audio_unlock();

    if (is_full) { // temporary read index
        pltf_log("+++ Audio Queue Full!\n");
    } else {
        APP->aud.cmds[APP->aud.i_cmd_w_tmp] = c;
        APP->aud.i_cmd_w_tmp                = i;
    }
}

static inline u32 aud_cmd_next_index(u32 i)
{
    return ((i + 1) & (NUM_AUD_CMD_QUEUE - 1));
}

// Called by gameplay thread/context
void aud_cmd_queue_commit()
{
#ifdef PLTF_PD_HW
    // data memory barrier; prevent memory access reordering
    // ensures all commands are fully written before making them visible
    // to the audio context via the write index
    // -> needed because of interrupts
    __asm volatile("dmb");
#endif
    pltf_audio_lock();
    APP->aud.i_cmd_w = APP->aud.i_cmd_w_tmp;
    pltf_audio_unlock();
    sizeof(aud_cmd_s);
}

void aud_set_lowpass(i32 lp)
{
    aud_cmd_s cmd   = {AUD_CMD_LOWPASS};
    cmd.c.lowpass.v = lp;
    aud_push_cmd(cmd);
}

void mus_play(const char *fname)
{
    aud_cmd_s cmd         = {AUD_CMD_MUS_PLAY, AUD_CMD_PRIORITY_MUS_PLAY};
    cmd.c.mus_play.hash   = wad_hash(fname);
    cmd.c.mus_play.vol_q8 = 128;
    aud_push_cmd(cmd);
}

static void muschannel_stop(muschannel_s *mc)
{
    qoa_mus_end(&mc->qoa_str);
}

b32 snd_instance_play(snd_s s, f32 vol, f32 pitch)
{
    if (APP->aud.snd_playing_disabled) return 0;
    if (!s.dat) return 0;

    i32 vol_q8   = (i32)(vol * 256.5f);
    i32 pitch_q8 = (i32)(pitch * 256.5f);
    if (vol_q8 == 0) return 0;

    aud_cmd_s cmd           = {0};
    cmd.type                = AUD_CMD_SND_PLAY;
    cmd.c.snd_play.snd      = s;
    cmd.c.snd_play.pitch_q8 = pitch_q8;
    cmd.c.snd_play.vol_q8   = vol_q8;
    aud_push_cmd(cmd);
    return 1;
}

void sndchannel_stop(sndchannel_s *ch)
{
}