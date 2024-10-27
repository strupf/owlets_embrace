// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/aud.h"
#include "adpcm.h"
#include "gamedef.h"
#include "util/mathfunc.h"

AUD_s AUD;

static void       aud_cmds_flush();
static void       aud_push_cmd(aud_cmd_s c);
static inline u32 aud_cmd_next_index(u32 i);
//
static void       muschannel_playback(muschannel_s *mc, i16 *lb, i16 *rb, i32 len);
static void       muschannel_playback_part(muschannel_s *mc, i16 *lb, i16 *rb, i32 len);
static void       muschannel_stop(muschannel_s *mc);
static void       sndchannel_playback(sndchannel_s *sc, i16 *lbuf, i16 *rbuf, i32 len);
static void       sndchannel_stop(sndchannel_s *ch);

void aud_audio(i16 *lbuf, i16 *rbuf, i32 len)
{

    // assumption: provided buffers are 0 filled
    aud_cmds_flush();

    for (i32 n = 0; n < NUM_MUSCHANNEL; n++) {
        muschannel_s *ch = &AUD.muschannel[n];
        ch->adpcm.vol_q8 = 256;
        muschannel_playback(ch, lbuf, rbuf, len);
    }

    for (i32 n = 0; n < NUM_SNDCHANNEL; n++) {
        sndchannel_s *ch = &AUD.sndchannel[n];
        sndchannel_playback(ch, lbuf, rbuf, len);
    }

    if (AUD.lowpass) {
        i16 *b = lbuf;
        for (i32 n = 0; n < len; n++, b++) {
            AUD.lowpass_acc += ((i32)*b - AUD.lowpass_acc) >> AUD.lowpass;
            *b = (i16)AUD.lowpass_acc;
        }
    }
}

// Called by audio thread/context
static void aud_cmds_flush()
{
    while (AUD.i_cmd_r != AUD.i_cmd_w) {
        aud_cmd_s cmd_u = AUD.cmds[AUD.i_cmd_r];
        AUD.i_cmd_r     = aud_cmd_next_index(AUD.i_cmd_r);

        switch (cmd_u.type) {
        case AUD_CMD_SND_PLAY: {
            aud_cmd_snd_play_s *c = &cmd_u.c.snd_play;

            for (i32 i = 0; i < NUM_SNDCHANNEL; i++) {
                sndchannel_s *ch  = &AUD.sndchannel[i];
                adpcm_s      *pcm = &ch->adpcm;
                if (ch->snd_iID) continue;

                ch->snd_iID   = c->iID;
                pcm->data     = c->snd.buf;
                pcm->len      = c->snd.len;
                pcm->vol_q8   = c->vol_q8;
                pcm->data_pos = 0;
                adpcm_set_pitch(pcm, c->pitch_q8);
                adpcm_reset_to_start(pcm);
                break;
            }
            break;
        }
        case AUD_CMD_SND_MODIFY: {
            aud_cmd_snd_modify_s *c  = &cmd_u.c.snd_modify;
            sndchannel_s         *sc = NULL;

            for (i32 i = 0; i < NUM_SNDCHANNEL; i++) {
                sndchannel_s *sci = &AUD.sndchannel[i];
                if (sci->snd_iID == c->iID) {
                    sc = sci;
                    break;
                }
            }

            if (sc == NULL) break;

            if (c->stop) { // stop
                sndchannel_stop(sc);
                break;
            }
            sc->adpcm.vol_q8 = c->vol_q8;
            break;
        }
        case AUD_CMD_MUS_PLAY: {
            aud_cmd_mus_play_s *c  = &cmd_u.c.mus_play;
            muschannel_s       *mc = &AUD.muschannel[0];
            muschannel_stop(mc);

            char mfile[64];
            str_cpy(mfile, FILEPATH_MUS);
            str_append(mfile, c->mus_name);
            str_append(mfile, FILEEXTENSION_AUD);

            void *f = pltf_file_open_r(mfile);
            if (!f) {
                pltf_log("+++ Cant open music file: %s\n", mfile);
                break;
            }
            str_cpy(mc->mus_name, c->mus_name);
            adpcm_s *pcm         = &mc->adpcm;
            u32      num_samples = 0;
            pltf_file_r(f, &num_samples, sizeof(u32));
            pcm->data            = mc->chunk;
            pcm->len             = num_samples;
            pcm->vol_q8          = c->vol_q8;
            mc->stream           = f;
            mc->looping          = 1;
            mc->total_bytes_file = sizeof(u32) + ((num_samples + 1) >> 1);
            adpcm_set_pitch(pcm, 256);
            adpcm_reset_to_start(pcm);
            break;
        }
        case AUD_CMD_LOWPASS: {
            aud_cmd_lowpass_s *c = &cmd_u.c.lowpass;
            AUD.lowpass          = c->v;
            break;
        }
        default: break;
        }
    }
}

void aud_allow_playing_new_snd(bool32 enabled)
{
    AUD.snd_playing_disabled = !enabled;
}

// Called by gameplay thread/context
static void aud_push_cmd(aud_cmd_s c)
{
    // temporary write index
    // peek new position and see if the queue is full
    u32 i = aud_cmd_next_index(AUD.i_cmd_w_tmp);
    pltf_audio_lock();
    bool32 is_full = (i == AUD.i_cmd_r);
    pltf_audio_unlock();

    if (is_full) { // temporary read index
        pltf_log("+++ Audio Queue Full!\n");

        // TODO: scan queue and see if we can drop a less important command
    } else {
        AUD.cmds[AUD.i_cmd_w_tmp] = c;
        AUD.i_cmd_w_tmp           = i;
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
    ASM("dmb");
#endif
    pltf_audio_lock();
    AUD.i_cmd_w = AUD.i_cmd_w_tmp;
    pltf_audio_unlock();
}

void aud_set_lowpass(i32 lp)
{
    aud_cmd_s cmd   = {0};
    cmd.c.lowpass.v = lp;
    aud_push_cmd(cmd);
}

static void muschannel_playback(muschannel_s *mc, i16 *lb, i16 *rb, i32 len)
{
    if (!mc->stream) return;

    adpcm_s *pcm = &mc->adpcm;
    i32      l   = min_i32(len, pcm->len_pitched - pcm->pos_pitched - 1);
    muschannel_playback_part(mc, lb, rb, l);

    if (pcm->pos_pitched < pcm->len_pitched - 1) return;

    if (mc->looping) { // loop back to start
        adpcm_reset_to_start(pcm);
        pltf_file_seek_set(mc->stream, sizeof(u32));
        muschannel_playback_part(mc, &lb[l], rb ? &rb[l] : NULL, len - l);
    } else {
        muschannel_stop(mc);
    }
}

static void muschannel_playback_part(muschannel_s *mc, i16 *lb, i16 *rb, i32 len)
{
    if (len <= 0) return;

    adpcm_s *pcm  = &mc->adpcm;
    pcm->data_pos = 0;

#if 1 // music can't be pitched
    AUD_MUS_ASSERT(pcm->pos_pitched == pcm->pos);
    u32 pos_new = pcm->pos + len;
    u32 bneeded = (pos_new - pcm->pos + (pcm->nibble == 0)) >> 1;
#else
    u32 pos_new = (((pcm->pos_pitched + len) * pcm->ipitch_q8) >> 8);
    u32 bneeded = (pos_new - pcm->pos + (pcm->nibble == 0)) >> 1;
#endif

    // amount of new sample bytes needed for this call
    if (bneeded) {
        u32 ft    = pltf_file_tell(mc->stream);
        u32 fnewt = ft + bneeded;
        AUD_MUS_ASSERT(fnewt <= mc->total_bytes_file);
        AUD_MUS_ASSERT(bneeded <= sizeof(mc->chunk));
        pltf_file_r(mc->stream, mc->chunk, bneeded);
    }

#ifdef AUD_MUS_DEBUG
    const u32 data_pos_prev = pcm->data_pos;
#endif

    if (pcm->vol_q8) {
        adpcm_playback(pcm, lb, rb, len);
    } else {
        // save some cycles on silent parallel music channels
        // adpcm_playback_nonpitch_silent(pcm, len);
        adpcm_playback_nonpitch(pcm, lb, rb, len);
    }
#ifdef AUD_MUS_DEBUG
    AUD_MUS_ASSERT((pcm->data_pos - data_pos_prev) == bneeded);
    AUD_MUS_ASSERT(pcm->pos == pos_new);
    AUD_MUS_ASSERT(pcm->data_pos == bneeded);
    AUD_MUS_ASSERT(pcm->pos_pitched == pcm->pos);
#endif
}

void mus_play(const char *fname)
{
    aud_cmd_s cmd = {AUD_CMD_MUS_PLAY, AUD_CMD_PRIORITY_MUS_PLAY};
    str_cpy(cmd.c.mus_play.mus_name, fname);
    cmd.c.mus_play.vol_q8 = 128;
    aud_push_cmd(cmd);
}

static void muschannel_stop(muschannel_s *mc)
{
    if (!mc->stream) return;
    pltf_file_close(mc->stream);
    mc->stream = NULL;
    mclr(mc->mus_name, sizeof(mc->mus_name));
}

static void sndchannel_playback(sndchannel_s *sc, i16 *lbuf, i16 *rbuf, i32 len)
{
    if (!sc->snd_iID) return;
    adpcm_s *pcm = &sc->adpcm;
    AUD_MUS_ASSERT(0 < pcm->len_pitched);
    AUD_MUS_ASSERT(pcm->pos_pitched < pcm->len_pitched);
    i32 l = min_i32(len, pcm->len_pitched - pcm->pos_pitched - 1);
    adpcm_playback(pcm, lbuf, rbuf, l);
    AUD_MUS_ASSERT(pcm->pos_pitched < pcm->len_pitched);

    if (pcm->pos_pitched == pcm->len_pitched - 1) {
        sc->snd_iID = 0;
    }
}

u32 snd_instance_play(snd_s s, f32 vol, f32 pitch)
{
    if (AUD.snd_playing_disabled) return 0;
    if (!s.buf || s.len == 0) return 0;

    i32 vol_q8   = (i32)(vol * 256.5f);
    i32 pitch_q8 = (i32)(pitch * 256.5f);
    if (vol_q8 == 0) return 0;

    AUD.snd_iID++;
    if (AUD.snd_iID == 0) {
        AUD.snd_iID = 1;
    }

    aud_cmd_s cmd           = {AUD_CMD_SND_PLAY};
    cmd.c.snd_play.snd      = s;
    cmd.c.snd_play.iID      = AUD.snd_iID;
    cmd.c.snd_play.pitch_q8 = pitch_q8;
    cmd.c.snd_play.vol_q8   = vol_q8;
    aud_push_cmd(cmd);
    return AUD.snd_iID;
}

void snd_instance_stop(u32 snd_iID)
{
    aud_cmd_s cmd         = {AUD_CMD_SND_MODIFY};
    cmd.c.snd_modify.iID  = snd_iID;
    cmd.c.snd_modify.stop = 1;
    aud_push_cmd(cmd);
}

void snd_instance_set_vol(u32 snd_iID, f32 vol)
{
    aud_cmd_s cmd           = {AUD_CMD_SND_MODIFY};
    cmd.c.snd_modify.iID    = snd_iID;
    cmd.c.snd_modify.vol_q8 = (i32)(vol * 256.5f);
    aud_push_cmd(cmd);
}

void sndchannel_stop(sndchannel_s *ch)
{
    ch->adpcm.data = NULL;
    ch->snd_iID    = 0;
}

snd_s snd_load(const char *pathname, alloc_s ma)
{
    snd_s snd = {0};

    void *f = pltf_file_open_r(pathname);
    if (!f) {
        pltf_log("+++ ERR: can't open file for snd %s\n", pathname);
        return snd;
    }

    u32 num_samples = 0;
    pltf_file_r(f, &num_samples, sizeof(u32));
    u32 bytes = (num_samples + 1) >> 1;

    void *buf = ma.allocf(ma.ctx, bytes);
    if (!buf) {
        pltf_log("+++ ERR: can't allocate memory for snd %s\n", pathname);
        pltf_file_close(f);
        return snd;
    }

    pltf_file_r(f, buf, bytes);
    pltf_file_close(f);

    snd.buf = (u8 *)buf;
    snd.len = num_samples;
    return snd;
}
