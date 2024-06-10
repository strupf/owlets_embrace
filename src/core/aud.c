// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/aud.h"
#include "gamedef.h"

enum {
    MUS_FADE_NONE,
    MUS_FADE_OUT,
    MUS_FADE_IN,
};

AUD_s AUD;

void sndchannel_repitch(sndchannel_s *ch, f32 pitch);
void sndchannel_play(sndchannel_s *ch, i16 *lbuf, i32 len);
void muschannel_update_chunk(muschannel_s *ch, i32 samples);
void muschannel_fillbuf(muschannel_s *ch, i16 *buf, i32 len);
void muschannel_stream(muschannel_s *ch, i16 *buf, i32 len);

void aud_init()
{
    AUD.snd_pitch = 1.f;
}

void aud_update()
{
    switch (AUD.mus_fade) {
    case MUS_FADE_NONE:
        AUD.muschannel.vol_q8 = AUD.muschannel.trg_vol_q8;
        break;
    case MUS_FADE_OUT: {
        AUD.mus_fade_ticks--;
        mus_set_vol_q8((AUD.mus_fade_ticks * AUD.muschannel.trg_vol_q8) / AUD.mus_fade_ticks_max);
        if (AUD.mus_fade_ticks > 0) break;

        if (!mus_play(AUD.mus_new)) {
            AUD.mus_fade = MUS_FADE_NONE;
            break;
        }
        AUD.mus_fade           = MUS_FADE_IN;
        AUD.mus_fade_ticks     = AUD.mus_fade_in;
        AUD.mus_fade_ticks_max = AUD.mus_fade_ticks;
        break;
    }
    case MUS_FADE_IN: {
        AUD.mus_fade_ticks--;
        mus_set_vol_q8(AUD.muschannel.trg_vol_q8 - ((AUD.mus_fade_ticks * AUD.muschannel.trg_vol_q8) / AUD.mus_fade_ticks_max));

        if (AUD.mus_fade_ticks > 0) break;

        mus_set_vol_q8(AUD.muschannel.trg_vol_q8);
        AUD.mus_fade = MUS_FADE_NONE;
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
        if (!ch->wavbuf) continue;

        sndchannel_repitch(ch, pitch);
    }
#ifdef PLTF_SDL
    pltf_sdl_audio_unlock();
#endif
}

void aud_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
    // muschannel sets buffer to music or to 0 if no music
    muschannel_stream(&AUD.muschannel, lbuf, len);
    for (i32 n = 0; n < NUM_SNDCHANNEL; n++) {
        sndchannel_s *ch = &AUD.sndchannel[n];
        if (ch->wavbuf) {
            sndchannel_play(ch, lbuf, len);
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

void aud_allow_playing_new_snd(bool32 enabled)
{
    AUD.snd_playing_disabled = !enabled;
}

// loading a custom 8-bit 44100Hz mono wav file format
snd_s snd_load(const char *pathname, alloc_s ma)
{
    snd_s snd = {0};

    void *f = pltf_file_open_r(pathname);
    if (!f) {
        pltf_log("+++ ERR: can't open file for snd %s\n", pathname);
        return snd;
    }

    u32 audlen;
    pltf_file_r(f, &audlen, sizeof(audlen));

    void *buf = ma.allocf(ma.ctx, audlen);
    if (!buf) {
        pltf_log("+++ ERR: can't allocate memory for snd %s\n", pathname);
        pltf_file_close(f);
        return snd;
    }

    pltf_file_r(f, buf, audlen);
    pltf_file_close(f);

    snd.buf = (i8 *)buf;
    snd.len = audlen;
    return snd;
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
        if (ch->wavbuf) continue;

        f32 p              = pitch * AUD.snd_pitch;
        ch->wavbuf         = s.buf;
        ch->wavlen         = s.len;
        ch->vol_q8         = (i32)(vol * 256.f);
        ch->wavpos         = 0;
        ch->wavlen_pitched = (u32)((f32)s.len * p);
        ch->ipitch_q8      = (i32)(256.f / p);
        ch->pitch          = p;
        break;
    }
#ifdef PLTF_SDL
    pltf_sdl_audio_unlock();
#endif
}

void sndchannel_repitch(sndchannel_s *ch, f32 pitch)
{
    ch->wavpos         = (i32)((f32)ch->wavpos * pitch / ch->pitch);
    ch->wavlen_pitched = (i32)((f32)ch->wavlen * pitch);
    ch->ipitch_q8      = (i32)(256.f / pitch);
    ch->pitch          = pitch;
}

void sndchannel_play(sndchannel_s *ch, i16 *lbuf, i32 len)
{
    i32 lmax = ch->wavlen_pitched - ch->wavpos;
    i32 l    = min_i32(len, lmax);

    i16 *buf = lbuf;
    for (i32 n = 0; n < l; n++) {
        i32 i = (i32)((ch->wavpos++ * ch->ipitch_q8) >> 8);
        assert(i < ch->wavlen);
        i32 v = (i32)*buf + ((i32)ch->wavbuf[i] * ch->vol_q8); // i8 * Q8 -> i16
#if AUD_CLAMP
        v = i16_sat(v);
#endif
        *buf++ = v;
    }

    if (lmax <= len) { // last part of wav, stop after this
        ch->wavbuf = NULL;
    }
}

bool32 mus_play(const char *filename)
{
    muschannel_s *ch = &AUD.muschannel;
    mus_stop();

    // loading a custom 8-bit 44100Hz mono wav file format
    void *f = pltf_file_open_r(filename);
    if (!f) {
        return 0;
    }
#ifdef PLTF_SDL
    pltf_sdl_audio_lock();
#endif
    str_cpy(ch->filename, filename);
    u32 audlen;
    pltf_file_r(f, &audlen, sizeof(u32));

    ch->stream    = f;
    ch->datapos   = pltf_file_tell(f);
    ch->streamlen = audlen;
    ch->streampos = 0;
    ch->looping   = 1;
    muschannel_update_chunk(ch, 0);
#ifdef PLTF_SDL
    pltf_sdl_audio_unlock();
#endif
    return 1;
}

void mus_fade_to(const char *pathname, i32 ticks_out, i32 ticks_in)
{
    if (mus_playing() && str_eq(AUD.muschannel.filename, pathname)) return;
    AUD.mus_fade_in = ticks_in;
    AUD.mus_fade    = MUS_FADE_OUT;
    if (mus_playing()) {
        AUD.mus_fade_ticks_max = ticks_out;
    } else {
        AUD.mus_fade_ticks_max = 1;
    }
    AUD.mus_fade_ticks = AUD.mus_fade_ticks_max;
    str_cpy(AUD.mus_new, pathname);
}

void mus_stop()
{
    muschannel_s *ch = &AUD.muschannel;
    if (ch->stream) {
#ifdef PLTF_SDL
        pltf_sdl_audio_lock();
#endif
        pltf_file_close(ch->stream);
        ch->stream = NULL;
        mset(ch->filename, 0, sizeof(ch->filename));
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
    AUD.muschannel.vol_q8 = vol_q8;
}

void mus_set_vol(f32 vol)
{
    mus_set_vol_q8((i32)(256.f * vol));
}

void mus_set_trg_vol(f32 vol)
{
    AUD.muschannel.trg_vol_q8 = (i32)(256.f * vol);
}

void muschannel_stream(muschannel_s *ch, i16 *buf, i32 len)
{
    if (!ch->stream) {
        mset(buf, 0, sizeof(i16) * len);
        return;
    }

    i32 lmax = ch->streamlen - ch->streampos;
    i32 l    = min_i32(len, lmax);

    muschannel_update_chunk(ch, len);
    muschannel_fillbuf(ch, buf, l);

    ch->streampos += l;
    if (ch->streamlen <= ch->streampos) { // at the end of the song
        i32 samples_left = len - l;
        if (ch->looping) { // fill remainder of buffer and restart
            ch->streampos = samples_left;
            muschannel_update_chunk(ch, 0);
            muschannel_fillbuf(ch, &buf[l], samples_left);
        } else {
            mset(&buf[l], 0, samples_left * sizeof(i16));
            mus_stop();
        }
    }
}

// update loaded music chunk if we are running out of samples
void muschannel_update_chunk(muschannel_s *ch, i32 samples)
{
    i32 samples_chunked = MUSCHUNK_SAMPLES - ch->chunkpos;
    if (0 < samples && samples <= samples_chunked) return;

    // refill music buffer from file
    // place chunk beginning right at streampos
    pltf_file_seek_set(ch->stream, ch->datapos + ch->streampos * sizeof(i8));
    i32 samples_left    = ch->streamlen - ch->streampos;
    i32 samples_to_read = min_i(MUSCHUNK_SAMPLES, samples_left);

    pltf_file_r(ch->stream, ch->chunk, sizeof(i8) * samples_to_read);
    ch->chunkpos = 0;
}

void muschannel_fillbuf(muschannel_s *ch, i16 *buf, i32 len)
{
    i16 *b = buf;
    i8  *c = &ch->chunk[ch->chunkpos];
    ch->chunkpos += len;
    for (i32 n = 0; n < len; n++) {
        i32 vv = (i32)*c++ * ch->vol_q8; // i8 * Q8 -> Q16
        *b++   = vv;
    }
}
