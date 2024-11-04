// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "aud.h"
#include "gamedef.h"
#include "qoa.h"
#include "util/mathfunc.h"

AUD_s AUD;

static void       aud_cmds_flush();
static void       aud_push_cmd(aud_cmd_s c);
static inline u32 aud_cmd_next_index(u32 i);
//
static void       muschannel_stop(muschannel_s *mc);
static void       sndchannel_stop(sndchannel_s *ch);

void aud_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
    // assumption: provided buffers are 0 filled
    aud_cmds_flush();

    for (i32 n = 0; n < NUM_MUSCHANNEL; n++) {
        muschannel_s *ch = &AUD.muschannel[n];
        if (qoa_stream_active(&ch->qoa_str)) {
            qoa_stream(&ch->qoa_str, lbuf, len);
        }
    }

    for (i32 n = 0; n < NUM_SNDCHANNEL; n++) {
        sndchannel_s *ch = &AUD.sndchannel[n];
        if (qoa_data_active(&ch->qoa_dat)) {
            qoa_data_play(&ch->qoa_dat, lbuf, len);
        }
    }

    if (AUD.lowpass) {
        i16 *b = lbuf;
        for (i32 n = 0; n < len; n++) {
            AUD.lowpass_acc += ((i32)*b - AUD.lowpass_acc) >> AUD.lowpass;
            *b++ = (i16)AUD.lowpass_acc;
        }
    }
}

static void aud_cmd_execute(aud_cmd_s cmd_u);

// Called by audio thread/context
static void aud_cmds_flush()
{
    while (AUD.i_cmd_r != AUD.i_cmd_w) {
        aud_cmd_s cmd_u = AUD.cmds[AUD.i_cmd_r];
        AUD.i_cmd_r     = aud_cmd_next_index(AUD.i_cmd_r);
        aud_cmd_execute(cmd_u);
    }
}

static void aud_cmd_execute(aud_cmd_s cmd_u)
{
    switch (cmd_u.type) {
    default: break;
    case AUD_CMD_SND_PLAY: {
        aud_cmd_snd_play_s *c = &cmd_u.c.snd_play;

        for (i32 i = 0; i < NUM_SNDCHANNEL; i++) {
            sndchannel_s *ch = &AUD.sndchannel[i];
            qoa_data_s   *q  = &ch->qoa_dat;
            if (qoa_data_active(q)) continue;

            ch->snd_iID = c->iID;
            qoa_data_start(q, c->snd.num_samples, c->snd.dat, c->pitch_q8, c->vol_q8, 0);
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
        break;
    }
    case AUD_CMD_MUS_PLAY: {
        aud_cmd_mus_play_s *c  = &cmd_u.c.mus_play;
        muschannel_s       *mc = &AUD.muschannel[0];
        qoa_stream_s       *q  = &mc->qoa_str;
        char                mfile[64];
        str_cpy(mfile, FILEPATH_MUS);
        str_append(mfile, c->mus_name);
        str_append(mfile, FILEEXTENSION_AUD);
        str_cpy(mc->mus_name, c->mus_name);
        if (qoa_stream_active(q)) {
            qoa_stream_end(q);
        }
        qoa_stream_start(q, mfile, 0, 256);
        break;
    }
    case AUD_CMD_LOWPASS: {
        aud_cmd_lowpass_s *c = &cmd_u.c.lowpass;
        AUD.lowpass          = c->v;
        break;
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

u32 snd_instance_play(snd_s s, f32 vol, f32 pitch)
{
    if (AUD.snd_playing_disabled) return 0;
    if (!s.dat) return 0;

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
}

snd_s snd_load(const char *pathname, alloc_s ma)
{
    snd_s snd = {0};

    void *f = pltf_file_open_r(pathname);
    if (!f) {
        pltf_log("+++ ERR: can't open file for snd %s\n", pathname);
        return snd;
    }

    qoa_file_header_s head = {0};
    pltf_file_r(f, &head, sizeof(qoa_file_header_s));
    u32 bytes = head.num_slices * sizeof(u64);

    void *buf = ma.allocf(ma.ctx, bytes);
    if (!buf) {
        pltf_log("+++ ERR: can't allocate memory for snd %s\n", pathname);
        pltf_file_close(f);
        return snd;
    }
    assert(((uptr)buf & 7) == 0);
    pltf_file_r(f, buf, bytes);
    pltf_file_close(f);

    snd.dat         = buf;
    snd.num_samples = head.num_samples;
    return snd;
}
