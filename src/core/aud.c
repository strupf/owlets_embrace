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
static void       muschannel_stop(muschannel_s *mc);

// no callback yet - can't be interruped
err32 aud_init()
{
    aud_s *aud    = &APP.aud;
    aud->v_mus_q8 = ((i32)SETTINGS.vol_mus << 7) / SETTINGS_VOL_MAX;
    aud->v_sfx_q8 = ((i32)SETTINGS.vol_sfx << 8) / SETTINGS_VOL_MAX;
#if 1
    aud->v_mus_q8 = 0;
#endif
#if 0
    aud->v_sfx_q8 = 0;
#endif
    return 0;
}

// removed from callback - can't be interruped anymore
void aud_destroy()
{
    for (i32 n = 0; n < NUM_MUSCHANNEL; n++) {
        qoa_mus_end(&APP.aud.muschannel[n].qoa_str);
    }
}

void aud_audio(aud_s *a, i16 *lbuf, i16 *rbuf, i32 len)
{
    // assumption: provided buffers are 0 filled

    // flush audio commands
    // catch up the read index to the write index in the
    // circular command buffer
    while (a->i_cmd_r != a->i_cmd_w) {
        aud_cmd_s cmd_u = a->cmds[a->i_cmd_r];
        a->i_cmd_r      = aud_cmd_next_index(a->i_cmd_r);
        aud_cmd_execute(cmd_u);
    }

    for (i32 n = 0; n < NUM_MUSCHANNEL; n++) {
        muschannel_s *c = &a->muschannel[n];
        qoa_mus_s    *q = &c->qoa_str;

        switch (c->fadestate) {
        case MUSCHANNEL_FADE_OUT: {
            c->t_fade += len;
            if (c->t_fade_out <= c->t_fade) {
                c->v_q8   = c->v_fade1;
                c->t_fade = 0;
                qoa_mus_end(q);
                void     *f;
                wad_el_s *wad_el;
                if (c->hash_queued && wad_open(c->hash_queued, &f, &wad_el)) {
                    c->fadestate   = MUSCHANNEL_FADE_IN;
                    c->hash_stream = c->hash_queued;
                    qoa_mus_start(q, f);
                    qoa_mus_set_loop(q, c->loop_s1, c->loop_s2);
                    if (c->start_at) {
                        qoa_mus_seek(q, c->start_at);
                    }
                } else {
                    c->fadestate = 0;
                }

                c->hash_queued = 0;
            } else {
                c->v_q8 = lerp_i32(c->v_fade0, c->v_fade1,
                                   c->t_fade, c->t_fade_out);
            }
            break;
        }
        case MUSCHANNEL_FADE_IN: {
            c->t_fade += len;

            if (c->t_fade_in <= c->t_fade) {
                c->v_q8      = c->v_fade2;
                c->t_fade    = 0;
                c->fadestate = 0;
            } else {
                c->v_q8 = lerp_i32(c->v_fade1, c->v_fade2,
                                   c->t_fade, c->t_fade_in);
            }
            break;
        }
        }

        qoa_mus(q, lbuf, rbuf, len, (i32)a->v_mus_q8 * (i32)c->v_q8);
    }

    for (i32 n = 0; n < NUM_SNDCHANNEL; n++) {
        sndchannel_s *ch = &a->sndchannel[n];
        if (!ch->snd_iID) continue;

        qoa_sfx_s *q = &ch->qoa_dat;

        if (ch->stop_ticks) {
            i32  ll = min_i32(len, ch->stop_ticks - ch->stop_tick);
            i16 *lb = lbuf;
            i16 *rb = rbuf;

            while (ll) {
                // subdivide to prevent clicking on quick stopping
                i32 dl = min_i32(ll, 16);

                if (!qoa_sfx_play(q, lb, rb, dl, a->v_sfx_q8)) {
                    ch->snd_iID = 0;
                    break;
                }

                ll -= dl;
                lb += dl;
                rb += dl;
                ch->stop_tick += dl;
                q->v_q8 = lerp_i32(ch->stop_v_q8, 0,
                                   ch->stop_tick, ch->stop_ticks);
            }
            if (ch->stop_ticks <= ch->stop_tick) {
                qoa_sfx_end(q);
                ch->snd_iID = 0;
            }
        } else {
            if (!qoa_sfx_play(q, lbuf, rbuf, len, a->v_sfx_q8)) {
                ch->snd_iID = 0;
            }
        }
    }

    // "fade" lowpass to target lowpass intensity to avoid clicking
    if (a->lowpass < a->lowpass_dst) {
        a->lowpass++;
    } else if (a->lowpass > a->lowpass_dst) {
        a->lowpass--;
    }

    if (a->lowpass) {
        i16 *lb = lbuf;
        i16 *rb = rbuf;
        for (i32 n = 0; n < len; n++) {
            a->lowpass_l += ((i32)*lb - (i32)a->lowpass_l) / a->lowpass;
            *lb++ = (i16)a->lowpass_l;
        }
        for (i32 n = 0; n < len; n++) {
            a->lowpass_r += ((i32)*rb - (i32)a->lowpass_r) / a->lowpass;
            *rb++ = (i16)a->lowpass_r;
        }
    }
}

static void aud_cmd_execute(aud_cmd_s cmd_u)
{
    switch (cmd_u.type) {
    default: break;
    case AUD_CMD_SND_MOD: {
        aud_cmd_snd_mod_s *c = &cmd_u.c.snd_mod;

        sndchannel_s *ch = 0;
        for (i32 i = 0; i < NUM_SNDCHANNEL; i++) {
            sndchannel_s *cht = &APP.aud.sndchannel[i];
            if (cht->snd_iID == c->iID) {
                ch = cht;
                break;
            }
        }

        if (!ch) break;
        qoa_sfx_s *q = &ch->qoa_dat;

        ch->stop_v_q8  = (q->v_q8 * c->vmod_q8) >> 8;
        ch->stop_ticks = c->stop_ticks;
        ch->stop_tick  = 0;
        break;
    }
    case AUD_CMD_SND_PLAY: {
        aud_cmd_snd_play_s *c = &cmd_u.c.snd_play;

        for (i32 i = 0; i < NUM_SNDCHANNEL; i++) {
            sndchannel_s *ch = &APP.aud.sndchannel[i];
            if (ch->snd_iID) continue;

            qoa_sfx_s *q = &ch->qoa_dat;
            qoa_sfx_start(q, c->snd.num_samples, c->snd.dat,
                          c->pitch_q8, c->vol_q8, c->repeat);
            ch->snd_iID    = c->iID;
            ch->stop_tick  = 0;
            ch->stop_ticks = 0;
            break;
        }
        break;
    }
    case AUD_CMD_MUS_VOL: {
        aud_cmd_mus_vol_s *c  = &cmd_u.c.mus_vol;
        muschannel_s      *mc = &APP.aud.muschannel[c->channelID];
        qoa_mus_s         *q  = &mc->qoa_str;
        if (qoa_mus_active(q)) {
            mc->fadestate = MUSCHANNEL_FADE_IN;
            mc->t_fade    = 0;
            mc->t_fade_in = c->ticks;
            mc->v_fade2   = c->vol_q8;
            mc->v_fade1   = mc->v_q8;
        }
        break;
    }
    case AUD_CMD_MUS_LOOP: {
        aud_cmd_mus_loop_s *c  = &cmd_u.c.mus_loop;
        muschannel_s       *mc = &APP.aud.muschannel[c->channelID];
        qoa_mus_s          *q  = &mc->qoa_str;
        if (qoa_mus_active(q)) {
            q->loop_s1 = c->loop_s1;
            q->loop_s2 = c->loop_s2;
        }
        break;
    }
    case AUD_CMD_MUS_PLAY: {
        aud_cmd_mus_play_s *c  = &cmd_u.c.mus_play;
        muschannel_s       *mc = &APP.aud.muschannel[c->channelID];
        qoa_mus_s          *q  = &mc->qoa_str;

        if (!qoa_mus_active(q) && c->hash == 0) break;

        mc->loop_s1     = c->loop_s1;
        mc->loop_s2     = c->loop_s2;
        mc->t_fade      = 0;
        mc->hash_queued = c->hash;
        mc->t_fade_in   = c->ticks_in;
        mc->v_fade1     = 0;
        mc->v_fade2     = c->vol_q8;
        mc->fadestate   = MUSCHANNEL_FADE_OUT;
        mc->start_at    = c->start_at;

        if (qoa_mus_active(q)) {
            mc->t_fade_out = c->ticks_out;
            mc->v_fade0    = mc->v_q8;
        } else {
            mc->v_fade0    = 0;
            mc->t_fade_out = 0;
        }
        break;
    }
    case AUD_CMD_STOP_ALL_SND: {
        for (i32 i = 0; i < NUM_SNDCHANNEL; i++) {
            sndchannel_s *ch = &APP.aud.sndchannel[i];
            if (ch->snd_iID) {
                ch->stop_ticks = 100;
                ch->stop_tick  = 0;
            }
        }
        break;
    }
    case AUD_CMD_LOWPASS: {
        aud_cmd_lowpass_s *c = &cmd_u.c.lowpass;
        APP.aud.lowpass_dst  = 1 << c->v;
        break;
    }
    }
}

void aud_allow_playing_new_snd(bool32 enabled)
{
    APP.aud.snd_playing_disabled = !enabled;
}

// Called by gameplay thread/context
static void aud_push_cmd(aud_cmd_s c)
{
    // temporary write index
    // peek new position and see if the queue is full
    u32 i = aud_cmd_next_index(APP.aud.i_cmd_w_tmp);
#if PLTF_SDL
    // lock and unlock thread mutex when using SDL
    pltf_sdl_audio_lock();
#endif
    bool32 is_full = (i == APP.aud.i_cmd_r);
#if PLTF_SDL
    pltf_sdl_audio_unlock();
#endif

    if (is_full) { // temporary read index
        // pltf_log("+++ Audio Queue Full!\n");
    } else {
        APP.aud.cmds[APP.aud.i_cmd_w_tmp] = c;
        APP.aud.i_cmd_w_tmp               = i;
    }
}

static inline u32 aud_cmd_next_index(u32 i)
{
    return ((i + 1) & (NUM_AUD_CMD_QUEUE - 1));
}

// Called by gameplay thread/context
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
    APP.aud.i_cmd_w = APP.aud.i_cmd_w_tmp;
#if PLTF_SDL
    pltf_sdl_audio_unlock();
#endif
}

void aud_set_lowpass(i32 lp)
{
    aud_cmd_s cmd   = {0};
    cmd.type        = AUD_CMD_LOWPASS;
    cmd.c.lowpass.v = lp;
    aud_push_cmd(cmd);
}

void mus_play_extv(const void *fname, u32 s1, u32 s2, i32 t_fade_out, i32 t_fade_in, i32 v_q8)
{
    mus_play_ext(0, fname, s1, s2, t_fade_out, t_fade_in, v_q8);
}

void mus_play_ext(i32 channelID, const void *fname, u32 s1, u32 s2, i32 t_fade_out, i32 t_fade_in, i32 v_q8)
{
    aud_cmd_s           cmd = {0};
    aud_cmd_mus_play_s *c   = &cmd.c.mus_play;
    cmd.type                = AUD_CMD_MUS_PLAY;
    c->channelID            = channelID;
    c->hash                 = wad_hash(fname);
    c->vol_q8               = v_q8;
    c->loop_s1              = s1;
    c->loop_s2              = (s2 * 44100) / 1000;
    c->ticks_in             = (t_fade_in * 44100) / 1000;
    c->ticks_out            = (t_fade_out * 44100) / 1000;
    aud_push_cmd(cmd);
}

void mus_play_extx(const void *fname, u32 start_at, u32 s1, u32 s2, i32 t_fade_out, i32 t_fade_in, i32 v_q8)
{
    aud_cmd_s           cmd = {0};
    aud_cmd_mus_play_s *c   = &cmd.c.mus_play;
    sizeof(aud_cmd_mus_play_s);
    cmd.type     = AUD_CMD_MUS_PLAY;
    c->channelID = 0;
    c->start_at  = start_at;
    c->hash      = wad_hash(fname);
    c->vol_q8    = v_q8;
    c->loop_s1   = s1;
    c->loop_s2   = (s2 * 44100) / 1000;
    c->ticks_in  = (t_fade_in * 44100) / 1000;
    c->ticks_out = (t_fade_out * 44100) / 1000;
    aud_push_cmd(cmd);
}

void mus_set_loop(i32 channelID, u32 s1, u32 s2)
{
    aud_cmd_s           cmd = {0};
    aud_cmd_mus_loop_s *c   = &cmd.c.mus_loop;
    cmd.type                = AUD_CMD_MUS_LOOP;
    c->channelID            = channelID;
    c->loop_s1              = (s1 * 44100) / 1000;
    c->loop_s2              = (s2 * 44100) / 1000;
    aud_push_cmd(cmd);
}

void mus_set_vol(u16 v_q8, i32 t_fade)
{
    mus_set_vol_ext(0, v_q8, t_fade);
}

void mus_set_vol_ext(i32 channelID, u16 v_q8, i32 t_fade)
{
    aud_cmd_s          cmd = {0};
    aud_cmd_mus_vol_s *c   = &cmd.c.mus_vol;
    cmd.type               = AUD_CMD_MUS_VOL;
    c->channelID           = channelID;
    c->ticks               = t_fade;
    c->vol_q8              = v_q8;
    aud_push_cmd(cmd);
}

void mus_play(const void *fname)
{
    mus_play_extv(fname, 0, 0, 0, 0, 256);
}

static void muschannel_stop(muschannel_s *mc)
{
    qoa_mus_end(&mc->qoa_str);
}

i32 snd_instance_play(snd_s s, f32 vol, f32 pitch)
{
    return snd_instance_play_ext(s, vol, pitch, 0);
}

i32 snd_instance_play_ext(snd_s s, f32 vol, f32 pitch, bool32 repeat)
{
    aud_s *a = &APP.aud;

    if (a->snd_playing_disabled) return 0;
    if (!s.dat) return 0;

    i32 vol_q8   = (i32)(vol * 256.5f);
    i32 pitch_q8 = (i32)(pitch * 256.5f);
    if (vol_q8 <= 4) return 0; // might as well not play it

    a->snd_iID = (a->snd_iID < I32_MAX ? a->snd_iID + 1 : 1);

    aud_cmd_s           cmd = {0};
    aud_cmd_snd_play_s *c   = &cmd.c.snd_play;
    cmd.type                = AUD_CMD_SND_PLAY;
    c->snd                  = s;
    c->pitch_q8             = pitch_q8;
    c->vol_q8               = vol_q8;
    c->iID                  = a->snd_iID;
    c->repeat               = repeat;
    aud_push_cmd(cmd);
    return a->snd_iID;
}

void snd_instance_stop(i32 iID)
{
    snd_instance_stop_fade(iID, 1, 256);
}

void snd_instance_stop_fade(i32 iID, i32 ms, i32 vmod_q8)
{
    if (iID == 0) return;
    aud_cmd_s          cmd = {0};
    aud_cmd_snd_mod_s *c   = &cmd.c.snd_mod;
    cmd.type               = AUD_CMD_SND_MOD;
    c->iID                 = iID;
    c->stop_ticks          = (ms * 44100) / 1000;
    c->vmod_q8             = vmod_q8;
    aud_push_cmd(cmd);
}

void aud_stop_all_snd_instances()
{
    aud_cmd_s cmd = {0};
    cmd.type      = AUD_CMD_STOP_ALL_SND;
    aud_push_cmd(cmd);
}