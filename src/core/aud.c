// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/aud.h"
#include "gamedef.h"
#include "sys/sys.h"

enum {
    MUS_FADE_NONE,
    MUS_FADE_OUT,
    MUS_FADE_IN,
};

AUD_s AUD;

static void sndchannel_play(sndchannel_s *ch, i16 *lbuf, int len);
static void muschannel_update_chunk(muschannel_s *ch, int samples);
static void muschannel_fillbuf(muschannel_s *ch, i16 *buf, int len);
static void muschannel_stream(muschannel_s *ch, i16 *buf, int len);

void aud_update()
{
    switch (AUD.mus_fade) {
    case MUS_FADE_NONE:
        AUD.muschannel.vol_q8 = AUD.muschannel.trg_vol_q8;
        break;
    case MUS_FADE_OUT: {
        AUD.mus_fade_ticks--;
        mus_set_vol((AUD.mus_fade_ticks * AUD.muschannel.trg_vol_q8) / AUD.mus_fade_ticks_max);
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
        mus_set_vol(AUD.muschannel.trg_vol_q8 - ((AUD.mus_fade_ticks * AUD.muschannel.trg_vol_q8) / AUD.mus_fade_ticks_max));

        if (AUD.mus_fade_ticks > 0) break;

        mus_set_vol(AUD.muschannel.trg_vol_q8);
        AUD.mus_fade = MUS_FADE_NONE;
        break;
    }
    }
}

void aud_mute(bool32 mute)
{
    AUD.mute = mute;
}

void aud_audio(i16 *buf, int len)
{
#ifdef SYS_SDL
    if (AUD.mute) {
        memset(buf, 0, sizeof(i16) * len);
        return;
    }
#endif

    muschannel_stream(&AUD.muschannel, buf, len);
    for (int n = 0; n < NUM_SNDCHANNEL; n++) {
        sndchannel_s *ch = &AUD.sndchannel[n];
        if (ch->wavedata) {
            sndchannel_play(ch, buf, len);
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

    void *f = sys_file_open(pathname, SYS_FILE_R);
    if (!f) {
        sys_printf("+++ ERR: can't open file for snd %s\n", pathname);
        return snd;
    }

    u32 audlen;
    sys_file_read(f, &audlen, sizeof(audlen));

    void *buf = ma.allocf(ma.ctx, audlen);
    if (!buf) {
        sys_printf("+++ ERR: can't allocate memory for snd %s\n", pathname);
        sys_file_close(f);
        return snd;
    }

    sys_file_read(f, buf, audlen);
    sys_file_close(f);

    snd.buf = (i8 *)buf;
    snd.len = audlen;
    return snd;
}

void snd_play(snd_s s, f32 vol, f32 pitch)
{
#ifdef SYS_SDL
    if (AUD.snd_playing_disabled) return;
#endif
    if (!s.buf || s.len == 0 || vol < .05f) return;

    for (int i = 0; i < NUM_SNDCHANNEL; i++) {
        sndchannel_s *ch = &AUD.sndchannel[i];
        if (ch->wavedata) continue;
        ch->wavedata       = s.buf;
        ch->wavelen_og     = s.len;
        ch->wavelen        = (int)((f32)s.len * pitch);
        ch->invpitch_q8    = (int)(256.f / pitch);
        ch->vol_q8         = (int)(vol * 256.f);
        ch->wavepos        = 0;
        ch->wavepos_inv_q8 = 0;

        break;
    }
}

static void sndchannel_play(sndchannel_s *ch, i16 *lbuf, int len)
{
    int lmax = ch->wavelen - ch->wavepos;
    int l    = min_i(len, lmax);

    ch->wavepos += l;
    i16 *buf = lbuf;
    for (int n = 0; n < l; n++) {
        int i = ch->wavepos_inv_q8 >> 8;
        ch->wavepos_inv_q8 += ch->invpitch_q8;
        assert(i < ch->wavelen_og);

        // i8 * Q8 -> i16
        i32 v = (i32)*buf + ((i32)ch->wavedata[i] * ch->vol_q8);
#if AUD_CLAMP
        v = clamp_i32(v, I16_MIN, I16_MAX);
#endif
        *buf++ = v;
    }

    if (lmax <= len) { // last part of wav, stop after this
        ch->wavedata = NULL;
    }
}

bool32 mus_play(const char *filename)
{
    muschannel_s *ch = &AUD.muschannel;
    mus_stop();

    // loading a custom 8-bit 44100Hz mono wav file format
    void *f = sys_file_open(filename, SYS_FILE_R);
    if (!f) {
        return 0;
    }
    strcpy(ch->filename, filename);
    u32 audlen;
    sys_file_read(f, &audlen, sizeof(u32));

    ch->stream    = f;
    ch->datapos   = sys_file_tell(f);
    ch->streamlen = audlen;
    ch->streampos = 0;
    ch->looping   = 1;
    muschannel_update_chunk(ch, 0);
    return 1;
}

void mus_fade_to(const char *pathname, int ticks_out, int ticks_in)
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
    str_cpys(AUD.mus_new, sizeof(AUD.mus_new), pathname);
}

void mus_stop()
{
    muschannel_s *ch = &AUD.muschannel;
    if (ch->stream) {
        sys_file_close(ch->stream);
        ch->stream = NULL;
        memset(ch->filename, 0, sizeof(ch->filename));
    }
}

bool32 mus_playing()
{
    return (AUD.muschannel.stream != NULL);
}

void mus_set_vol(int vol_q8)
{
    AUD.muschannel.vol_q8 = vol_q8;
}

void mus_set_trg_vol(int vol_q8)
{
    AUD.muschannel.trg_vol_q8 = vol_q8;
}

static void muschannel_stream(muschannel_s *ch, i16 *buf, int len)
{
    if (!ch->stream) {
        memset(buf, 0, sizeof(i16) * len);
        return;
    }

    int l = min_i(len, (int)(ch->streamlen - ch->streampos));
    muschannel_update_chunk(ch, len);
    muschannel_fillbuf(ch, buf, l);

    ch->streampos += l;
    if (ch->streampos >= ch->streamlen) { // at the end of the song
        int samples_left = len - l;
        if (ch->looping) { // fill remainder of buffer and restart
            ch->streampos = samples_left;
            muschannel_update_chunk(ch, 0);
            muschannel_fillbuf(ch, &buf[l], samples_left);
        } else {
            memset(&buf[l], 0, samples_left * sizeof(i16));
            mus_stop();
        }
    }
}

// update loaded music chunk if we are running out of samples
static void muschannel_update_chunk(muschannel_s *ch, int samples)
{
    int samples_chunked = MUSCHUNK_SAMPLES - ch->chunkpos;
    if (0 < samples && samples <= samples_chunked) return;

    // refill music buffer from file
    // place chunk beginning right at streampos
    sys_file_seek(ch->stream,
                  ch->datapos + ch->streampos * sizeof(i8),
                  SYS_FILE_SEEK_SET);
    int samples_left    = ch->streamlen - ch->streampos;
    int samples_to_read = min_i(MUSCHUNK_SAMPLES, samples_left);

    sys_file_read(ch->stream, ch->chunk, sizeof(i8) * samples_to_read);
    ch->chunkpos = 0;
}

static void muschannel_fillbuf(muschannel_s *ch, i16 *buf, int len)
{
    i16 *b = buf;
    i8  *c = &ch->chunk[ch->chunkpos];
    ch->chunkpos += len;
    for (int n = 0; n < len; n++) {
        *b++ = (i16)((i32)*c++ * ch->vol_q8); // i8 * Q8 -> Q16
    }
}
