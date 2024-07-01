// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/aud.h"
#include "gamedef.h"
#include "util/mathfunc.h"

enum {
    MUS_FADE_NONE,
    MUS_FADE_OUT,
    MUS_FADE_IN,
};

#if 0
#define AUD_MUS_ASSERT assert
#else
#define AUD_MUS_ASSERT(X)
#endif

AUD_s AUD;

void sndchannel_repitch(sndchannel_s *ch, f32 pitch);

void aud_init()
{
    AUD.snd_pitch = 1.f;
}

void aud_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
    mset(lbuf, 0, len);
    mus_stream(&AUD.muschannel, lbuf, rbuf, len);

    for (i32 n = 0; n < NUM_SNDCHANNEL; n++) {
        sndchannel_s *ch = &AUD.sndchannel[n];
        snd_playback(ch, lbuf, rbuf, len);
    }

    if (AUD.lowpass) {
        i16 *b = lbuf;
        for (i32 n = 0; n < len; n++) {
            AUD.lowpass_acc += ((i32)*b - AUD.lowpass_acc) >> AUD.lowpass;
            *b++ = (i16)AUD.lowpass_acc;
        }
    }
}

void aud_allow_playing_new_snd(bool32 enabled)
{
    AUD.snd_playing_disabled = !enabled;
}

void aud_update()
{
    muschannel_s *mc = &AUD.muschannel;
    switch (mc->fade) {
    case MUS_FADE_NONE:
        mc->adpcm.vol_q8 = mc->trg_vol_q8;
        break;
    case MUS_FADE_OUT: {
        mc->fade_ticks--;
        mus_set_vol_q8((mc->fade_ticks * mc->trg_vol_q8) / mc->fade_ticks_max);
        if (mc->fade_ticks > 0) break;

        if (!mus_play(mc->filename_new)) {
            mc->fade = MUS_FADE_NONE;
            break;
        }
        mc->fade           = MUS_FADE_IN;
        mc->fade_ticks     = mc->fade_in;
        mc->fade_ticks_max = mc->fade_ticks;
        break;
    }
    case MUS_FADE_IN: {
        mc->fade_ticks--;
        mus_set_vol_q8(mc->trg_vol_q8 - ((mc->fade_ticks * mc->trg_vol_q8) / mc->fade_ticks_max));

        if (mc->fade_ticks > 0) break;

        mus_set_vol_q8(mc->trg_vol_q8);
        mc->fade = MUS_FADE_NONE;
        break;
    }
    }
}

void aud_set_lowpass(i32 lp)
{
    AUD.lowpass = (lp <= 0 ? 0 : lp);
}

void aud_set_global_pitch(f32 pitch)
{
    AUD.snd_pitch = pitch;
#ifdef PLTF_SDL
    pltf_sdl_audio_lock();
#endif
    for (i32 i = 0; i < NUM_SNDCHANNEL; i++) {
        sndchannel_s *ch = &AUD.sndchannel[i];
        if (!ch->data) continue;
        NOT_IMPLEMENTED
        // sndchannel_repitch(ch, pitch);
    }
#ifdef PLTF_SDL
    pltf_sdl_audio_unlock();
#endif
}

// an ADPCM function to advance the sample decoding
// github.com/superctr/adpcm/blob/master/ymb_codec.c
static inline i16 adpcm_step(u32 step, i32 *history, i32 *step_size)
{
    static const u8 t_step[8] = {57, 57, 57, 57, 77, 102, 128, 153};

    i32 delta  = step & 7;
    i32 diff   = ((1 + (delta << 1)) * *step_size) >> 3;
    i32 newval = *history;
    i32 nstep  = (t_step[delta] * *step_size) >> 6;
    newval += ((step & 8) ? -diff : +diff);
    newval     = clamp_i32(newval, I16_MIN, I16_MAX);
    *step_size = clamp_i32(nstep, 127, 24576);
    *history   = newval;
    return newval;
}

static inline void adpcm_next(adpcm_s *pcm, u8 *buf, u32 *bufpos)
{
    if (!pcm->nibble) {
        pcm->curr_byte = buf[*bufpos];
        *bufpos        = *bufpos + 1;
    }
    u32 b = (pcm->curr_byte << pcm->nibble) >> 4;
    pcm->nibble ^= 4;
    pcm->curr_sample = adpcm_step(b, &pcm->hist, &pcm->step_size);
}

void adpcm_playback(adpcm_s *pcm, i16 *lbuf, i16 *rbuf, i32 len)
{
    i16 *lb = lbuf;

    for (i32 i = 0; i < len; i++) {
        u32 pos = (pcm->pos_pitched++ * pcm->ipitch_q8) >> 8;
        AUD_MUS_ASSERT(pos < pcm->len);
        while (pcm->pos < pos) { // can't savely skip any samples with ADPCM
            pcm->pos++;
            adpcm_next(pcm, pcm->data, &pcm->data_pos);
        }
        i32 v = (i32)*lb + ((pcm->curr_sample * pcm->vol_q8) >> 8);
#if AUD_CLAMP
        v = clamp_i32(v, I16_MIN, I16_MAX);
#endif
        *lb++ = v;
    }
}

void adpcm_reset_to_start(adpcm_s *pcm)
{
    pcm->step_size   = 127;
    pcm->hist        = 0;
    pcm->nibble      = 0;
    pcm->pos         = 0;
    pcm->pos_pitched = 0;
    pcm->curr_sample = 0;
    pcm->data_pos    = 0;
    pcm->curr_byte   = 0;
}

void adpcm_set_pitch(adpcm_s *pcm, i32 pitch_q8)
{
    if (pitch_q8 == 0) return;
    pcm->pitch_q8    = pitch_q8;
    pcm->ipitch_q8   = (256 << 8) / pcm->pitch_q8;
    pcm->len_pitched = (pcm->len * pcm->pitch_q8) >> 8;

    // highest position in pitched len shall not be out of bounce
    AUD_MUS_ASSERT((((pcm->len_pitched - 1) * pcm->ipitch_q8) >> 8) < pcm->len);
}

void mus_playback(muschannel_s *mc, i16 *lbuf, i16 *rbuf, i32 len)
{
    adpcm_s *pcm  = &mc->adpcm;
    pcm->data_pos = 0;
    u32 pos_new   = (((pcm->pos_pitched + len - 1) * pcm->ipitch_q8) >> 8);
    u32 bneeded   = (pos_new - pcm->pos + (pcm->nibble == 0)) >> 1;

    // amount of new sample bytes needed for this call
    if (bneeded) {
        AUD_MUS_ASSERT((pcm->pos >> 1) + bneeded <= pcm->len);
        AUD_MUS_ASSERT(bneeded <= sizeof(mc->chunk));
        pltf_file_r(mc->stream, mc->chunk, bneeded);
    }

    adpcm_playback(pcm, lbuf, rbuf, len);

    AUD_MUS_ASSERT(mc->adpcm.pos == pos_new);
    AUD_MUS_ASSERT(mc->adpcm.data_pos == bneeded);
}

void mus_stream(muschannel_s *mc, i16 *lbuf, i16 *rbuf, i32 len)
{
    if (!mc->stream) return;

    adpcm_s *pcm = &mc->adpcm;
    i32      l   = min_i32(len, pcm->len_pitched - pcm->pos_pitched);
    mus_playback(mc, lbuf, rbuf, l);

    if (pcm->pos_pitched == pcm->len_pitched) {
        i32 len_left = len - l;
        if (mc->looping) { // loop back to start
            adpcm_reset_to_start(pcm);
            pltf_file_seek_set(mc->stream, sizeof(u32));
            mus_playback(mc, &lbuf[l], rbuf ? &rbuf[l] : NULL, len_left);
        } else {
            mus_stop();
        }
    }
}

bool32 mus_play(const char *filename)
{
    muschannel_s *mc = &AUD.muschannel;
    mus_stop();

    void *f = pltf_file_open_r(filename);
    if (!f) {
        return 0;
    }
#ifdef PLTF_SDL
    pltf_sdl_audio_lock();
#endif
    str_cpy(mc->filename, filename);
    mc->stream      = f;
    u32 num_samples = 0;
    pltf_file_r(mc->stream, &num_samples, sizeof(u32));
    adpcm_s *pcm = &mc->adpcm;
    pcm->data    = mc->chunk;
    pcm->len     = num_samples;
    mc->looping  = 1;
    adpcm_set_pitch(pcm, 256);
    adpcm_reset_to_start(pcm);
#ifdef PLTF_SDL
    pltf_sdl_audio_unlock();
#endif
    return 1;
}

void mus_fade_to(const char *pathname, i32 ticks_out, i32 ticks_in)
{
    muschannel_s *mc = &AUD.muschannel;

    if (mus_playing() && str_eq(mc->filename, pathname)) return;
    mc->fade_in = ticks_in;
    mc->fade    = MUS_FADE_OUT;
    if (mus_playing()) {
        mc->fade_ticks_max = ticks_out;
    } else {
        mc->fade_ticks_max = 1;
    }
    mc->fade_ticks = mc->fade_ticks_max;
    str_cpy(mc->filename_new, pathname);
}

void mus_stop()
{
    muschannel_s *pcm = &AUD.muschannel;
    if (pcm->stream) {
#ifdef PLTF_SDL
        pltf_sdl_audio_lock();
#endif
        pltf_file_close(pcm->stream);
        pcm->stream = NULL;
        mset(pcm->filename, 0, sizeof(pcm->filename));
#ifdef PLTF_SDL
        pltf_sdl_audio_unlock();
#endif
    }
}

bool32 mus_playing()
{
    return (AUD.muschannel.stream != NULL);
}

void mus_set_vol_q8(i32 vol_q8)
{
    AUD.muschannel.adpcm.vol_q8 = vol_q8;
}

void mus_set_vol(f32 vol)
{
    mus_set_vol_q8((i32)(256.f * vol));
}

void mus_set_trg_vol(f32 vol)
{
    AUD.muschannel.trg_vol_q8 = (i32)(256.f * vol);
}

void snd_playback(sndchannel_s *sc, i16 *lbuf, i16 *rbuf, i32 len)
{
    if (!sc->data) return;

    adpcm_s *pcm = &sc->adpcm;
    i32      l   = min_i32(len, pcm->len_pitched - pcm->pos_pitched);
    adpcm_playback(pcm, lbuf, rbuf, l);

    if (pcm->pos_pitched == pcm->len_pitched) {
        sc->data  = NULL;
        pcm->data = NULL;
    }
}

void snd_play(snd_s s, f32 vol, f32 pitch)
{
    if (AUD.snd_playing_disabled) return;
    if (!s.buf || s.len == 0 || vol < .05f) return;

#ifdef PLTF_SDL
    pltf_sdl_audio_lock();
#endif
    for (i32 i = 0; i < NUM_SNDCHANNEL; i++) {
        sndchannel_s *ch = &AUD.sndchannel[i];
        if (ch->data) continue;

        adpcm_s *pcm  = &ch->adpcm;
        f32      p    = pitch * AUD.snd_pitch;
        ch->data      = s.buf;
        pcm->data     = s.buf;
        pcm->data_pos = 0;
        pcm->len      = s.len;
        pcm->vol_q8   = (i32)(256.f * vol);
        adpcm_set_pitch(pcm, (i32)(pitch * 256.f));
        adpcm_reset_to_start(pcm);
        break;
    }
#ifdef PLTF_SDL
    pltf_sdl_audio_unlock();
#endif
}

void sndchannel_repitch(sndchannel_s *ch, f32 pitch)
{
#if 0
    ch->wavpos         = (i32)((f32)ch->wavpos * pitch / ch->pitch);
    ch->wavlen_pitched = (i32)((f32)ch->wavlen * pitch);
    ch->ipitch_q8      = (i32)(256.f / pitch);
    ch->pitch          = pitch;
#endif
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
