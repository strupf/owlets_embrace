// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/aud.h"
#include "gamedef.h"
#include "sys/sys.h"

typedef struct {
    void *buf;
    int   fmt;
    int   len; // samples
    int   rate;
    int   bits;
} wav_s;

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
    case MUS_FADE_NONE: return;
    case MUS_FADE_OUT: {
        AUD.mus_fade_ticks--;
        mus_set_vol((AUD.mus_fade_ticks << 8) / AUD.mus_fade_ticks_max);
        if (AUD.mus_fade_ticks > 0) break;

        if (mus_play(AUD.mus_new) != 0) {
            AUD.mus_fade = MUS_FADE_NONE;
            return;
        }
        AUD.mus_fade           = MUS_FADE_IN;
        AUD.mus_fade_ticks     = AUD.mus_fade_in;
        AUD.mus_fade_ticks_max = AUD.mus_fade_ticks;
    } break;
    case MUS_FADE_IN: {
        AUD.mus_fade_ticks--;
        mus_set_vol(256 - ((AUD.mus_fade_ticks << 8) / AUD.mus_fade_ticks_max));
        if (AUD.mus_fade_ticks > 0) break;

        mus_set_vol(256);
        AUD.mus_fade = MUS_FADE_NONE;
    } break;
    }
}

void aud_mute(bool32 mute)
{
    AUD.mute = mute;
}

void aud_audio(i16 *buf, int len)
{
    if (AUD.mute) {
        memset(buf, 0, sizeof(i16) * len);
        return;
    }

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

// http://soundfile.sapp.org/doc/WaveFormat/
typedef struct {
    u32 chunkID;
    u32 chunksize;
    u32 format;
    u32 subchunk1ID;
    u32 subchunk1size;
    u16 audioformat;
    u16 numchannels;
    u32 samplerate;
    u32 byterate;
    u16 blockalign;
    u16 bitspersample;
    u32 subchunk2ID;   // "data"
    u32 subchunk2size; // file size
} wavheader_s;

static void  *wavfile_open(const char *filename, wavheader_s *wh);
static wav_s  wav_convert(wav_s src, int dst_fmt, alloc_s m);
static bool32 wav_fmt_get(int fmt, int *rate, int *size_per_sample);

static void *wavfile_open(const char *filename, wavheader_s *wh)
{
    assert(filename);
    if (!filename) return NULL;
    void *f = sys_file_open(filename, SYS_FILE_R);
    if (!f) return NULL;
    sys_file_read(f, wh, sizeof(wavheader_s));
    sys_file_seek(f, sizeof(wavheader_s), SYS_FILE_SEEK_SET);
    assert(wh->subchunk2ID == *((u32 *)"data"));
    return f;
}

wav_s wav_load(const char *filename, alloc_s ma)
{
    wav_s       wav = {0};
    wavheader_s wheader;
    void       *f = wavfile_open(filename, &wheader);
    if (!f) {
        sys_printf("+++ wav file err: %s\n", filename);
        return wav;
    }

    wav.buf = ma.allocf(ma.ctx, wheader.subchunk2size);
    if (wav.buf == NULL) {
        sys_printf("+++ wav mem error: %s\n", filename);
        goto WAVERR;
    }

    wav.len  = wheader.subchunk2size / wheader.blockalign;
    wav.bits = wheader.bitspersample;
    wav.rate = wheader.samplerate;
    sys_file_read(f, wav.buf, wheader.subchunk2size); // we don't check for errors here...
WAVERR:
    sys_file_close(f);
    return wav;
}

#if 0
static bool32 wav_fmt_get(int fmt, int *rate, int *size_per_sample)
{
    switch (fmt) {
    case WAV_FMT_22050_I8:
        *size_per_sample = 1;
        *rate            = 22050;
        return 1;
    case WAV_FMT_44100_I8:
        *size_per_sample = 1;
        *rate            = 44100;
        return 1;
    case WAV_FMT_22050_I16:
        *size_per_sample = 2;
        *rate            = 22050;
        return 1;
    case WAV_FMT_44100_I16:
        *size_per_sample = 2;
        *rate            = 44100;
        return 1;
    }
    return 0;
}

// cannot convert between arbitrary formats yet
// not tested yet!
static wav_s wav_convert(wav_s src, int dst_fmt, alloc_s m)
{
    wav_s w = {0};
    int   size_s, size_d;
    int   rate_s, rate_d;
    if (!wav_fmt_get(src.fmt, &rate_s, &size_s)) return w;
    if (!wav_fmt_get(dst_fmt, &rate_d, &size_d)) return w;

    int   samples_d = (src.len * rate_d) / rate_s;
    void *buf       = m.allocf(m.ctx, (usize)size_d * samples_d);
    if (!buf) return w;

    for (int i = 0; i < samples_d; i++) {
        int j = (i * rate_s) / rate_d;
        assert(0 <= i && i < src.len);

        i32 a = 0;
        switch (size_s) {
        case 1: a = (i32)((i8 *)src.buf)[j] << (size_d == 2 ? 8 : 0); break;
        case 2: a = (i32)((i16 *)src.buf)[j] >> (size_d == 1 ? 8 : 0); break;
        }

        switch (size_d) {
        case 1: ((i8 *)buf)[i] = a; break;
        case 2: ((i16 *)buf)[i] = a; break;
        }
    }

    w.buf = buf;
    w.len = samples_d;
    w.fmt = dst_fmt;
    return w;
}
#endif

snd_s snd_load(const char *pathname, alloc_s ma)
{
    wav_s w = wav_load(pathname, ma);
    snd_s r = {(i16 *)w.buf, w.len};
    return r;
}

void snd_play(snd_s s, f32 vol, f32 pitch)
{
    if (AUD.snd_playing_disabled) return;
    if (vol < 0.05f) return;
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
        i32 d = (i32)ch->wavedata[i];
        i32 v = (i32)*buf + ((d * ch->vol_q8) >> 8);
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

    wavheader_s wheader;
    void       *f = wavfile_open(filename, &wheader);
    if (!f) return 0;
    if (!(wheader.samplerate == 44100 &&
          wheader.bitspersample == 16 &&
          wheader.numchannels == 1)) {
        sys_printf("Music wrong format: %s\n", filename);
        sys_file_close(f);
        return 0;
    }

    strcpy(ch->filename, filename);

    ch->stream    = f;
    ch->datapos   = sys_file_tell(f);
    ch->streamlen = (int)(wheader.subchunk2size / sizeof(i16));
    ch->streampos = 0;
    ch->vol_q8    = 256;
    ch->looping   = 1;
    muschannel_update_chunk(ch, 0);
    return 1;
}

void mus_fade_to(const char *pathname, int ticks_out, int ticks_in)
{
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
                  ch->datapos + ch->streampos * sizeof(i16),
                  SYS_FILE_SEEK_SET);
    int samples_left    = ch->streamlen - ch->streampos;
    int samples_to_read = min_i(MUSCHUNK_SAMPLES, samples_left);

    sys_file_read(ch->stream, ch->chunk, sizeof(i16) * samples_to_read);
    ch->chunkpos = 0;
}

static void muschannel_fillbuf(muschannel_s *ch, i16 *buf, int len)
{
    i16 *b = buf;
    i16 *c = &ch->chunk[ch->chunkpos];

    ch->chunkpos += len;
    if (ch->vol_q8 == 256) {
        for (int n = 0; n < len; n++) {
            *b++ = *c++;
        }
    } else {
        for (int n = 0; n < len; n++) {
            *b++ = (*c++ * ch->vol_q8) >> 8;
        }
    }
}
