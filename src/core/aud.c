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

typedef struct {
    i16 *buf;
    int  len;
} wav_i16;

typedef struct {
    void *buf;
    int   fmt;
    int   len; // samples
} wav_s;

static wav_s wav_conv_to_i8_22050(wav_s src, alloc_s ma);
static wav_s wav_conv_to_i8_44100(wav_s src, alloc_s ma);
static wav_s wav_conv_to_i16_22050(wav_s src, alloc_s ma);
static wav_s wav_conv_to_i16_44100(wav_s src, alloc_s ma);

void aud_update()
{
    switch (AUD.mus_fade) {
    case MUS_FADE_NONE: return;
    case MUS_FADE_OUT: {
        AUD.mus_fade_ticks--;
        sys_set_mus_vol((AUD.mus_fade_ticks << 8) / AUD.mus_fade_ticks_max);
        if (AUD.mus_fade_ticks > 0) break;

        if (sys_mus_play(AUD.mus_new) != 0) {
            AUD.mus_fade = MUS_FADE_NONE;
            return;
        }
        AUD.mus_fade           = MUS_FADE_IN;
        AUD.mus_fade_ticks     = AUD.mus_fade_in;
        AUD.mus_fade_ticks_max = AUD.mus_fade_ticks;
    } break;
    case MUS_FADE_IN: {
        AUD.mus_fade_ticks--;
        sys_set_mus_vol(256 - ((AUD.mus_fade_ticks << 8) / AUD.mus_fade_ticks_max));
        if (AUD.mus_fade_ticks > 0) break;

        sys_set_mus_vol(256);
        AUD.mus_fade = MUS_FADE_NONE;
    } break;
    }
}

aud_snd_s aud_snd_load(const char *pathname, alloc_s ma)
{
    return sys_load_wav(pathname, ma);
}

void aud_snd_play(aud_snd_s s, f32 vol, f32 pitch)
{
    sys_wavdata_play(s, vol, pitch);
}

void aud_mus_fade_to(const char *pathname, int ticks_out, int ticks_in)
{
    AUD.mus_fade_in = ticks_in;
    AUD.mus_fade    = MUS_FADE_OUT;
    if (sys_mus_playing()) {
        AUD.mus_fade_ticks_max = ticks_out;
    } else {
        AUD.mus_fade_ticks_max = 1;
    }
    AUD.mus_fade_ticks = AUD.mus_fade_ticks_max;
    str_cpys(AUD.mus_new, sizeof(AUD.mus_new), pathname);
}

void aud_mus_stop()
{
    sys_mus_stop();
}

bool32 aud_mus_playing()
{
    return sys_mus_playing();
}

static void aud_set_freq(wave_s *w, int freq)
{
    if (0 <= freq && freq <= 16384) {
        w->incr = (u32)(((u64)freq << 32) / (u64)44100);
    } else {
        w->incr = 0;
    }
}

static void aud_set_midi(wave_s *w, int midi)
{
    // increment values for wave synth
    static const u32 incr_midi[128] = {
        0x000C265D, 0x000CDF51, 0x000DA344, 0x000E72DE,
        0x000F4ED0, 0x001037D7, 0x00112EB8, 0x00123448,
        0x00134965, 0x00146EFD, 0x0015A60A, 0x0016EF96,
        0x00184CBB, 0x0019BEA2, 0x001B4689, 0x001CE5BD,
        0x001E9DA1, 0x00206FAE, 0x00225D71, 0x00246891,
        0x002692CB, 0x0028DDFB, 0x002B4C15, 0x002DDF2D,
        0x00309976, 0x00337D45, 0x00368D12, 0x0039CB7A,
        0x003D3B43, 0x0040DF5C, 0x0044BAE3, 0x0048D122,
        0x004D2597, 0x0051BBF7, 0x0056982B, 0x005BBE5B,
        0x006132ED, 0x0066FA8B, 0x006D1A24, 0x007396F4,
        0x007A7686, 0x0081BEB9, 0x008975C6, 0x0091A244,
        0x009A4B2F, 0x00A377EE, 0x00AD3056, 0x00B77CB6,
        0x00C265DB, 0x00CDF516, 0x00DA3449, 0x00E72DE9,
        0x00F4ED0D, 0x01037D73, 0x0112EB8C, 0x01234489,
        0x0134965F, 0x0146EFDC, 0x015A60AD, 0x016EF96D,
        0x0184CBB6, 0x019BEA2D, 0x01B46892, 0x01CE5BD2,
        0x01E9DA1A, 0x0206FAE6, 0x0225D719, 0x02468912,
        0x02692CBF, 0x028DDFB9, 0x02B4C15A, 0x02DDF2DB,
        0x0309976D, 0x0337D45B, 0x0368D125, 0x039CB7A5,
        0x03D3B434, 0x040DF5CC, 0x044BAE33, 0x048D1225,
        0x04D2597F, 0x051BBF72, 0x056982B5, 0x05BBE5B7,
        0x06132EDB, 0x066FA8B6, 0x06D1A24A, 0x07396F4B,
        0x07A76868, 0x081BEB99, 0x08975C67, 0x091A244A,
        0x09A4B2FE, 0x0A377EE5, 0x0AD3056A, 0x0B77CB6E,
        0x0C265DB7, 0x0CDF516D, 0x0DA34494, 0x0E72DE96,
        0x0F4ED0D1, 0x1037D732, 0x112EB8CE, 0x12344894,
        0x134965FD, 0x146EFDCB, 0x15A60AD5, 0x16EF96DC,
        0x184CBB6F, 0x19BEA2DB, 0x1B468928, 0x1CE5BD2C,
        0x1E9DA1A3, 0x206FAE64, 0x225D719D, 0x24689129,
        0x2692CBFA, 0x28DDFB96, 0x2B4C15AA, 0x2DDF2DB9,
        0x309976DF, 0x337D45B6, 0x368D1251, 0x39CB7A58,
        0x3D3B4347, 0x40DF5CC9, 0x44BAE33A, 0x48D12252};

    // u32 incr = ((u64)(0.5 + 4294967296.0 * 440.0 * pow(2.0, (double)(i - 69) / 12.0))) / (u64)44100;
    w->incr = incr_midi[midi];
}

// maps u32 to sine, returns [-65536, +65536]
static inline int synth_wave_sine(u32 x)
{
    u32 i = ((x - 0x40000000U) >> 14);
    if ((i & 0xFFFF) == 0) return 0;
    int neg = 0;
    switch (i >> 16) {
    case 1: i = 0x20000 - i, neg = 1; break; // [65536, 131071]
    case 2: i = i - 0x20000, neg = 1; break; // [131072, 196607]
    case 3: i = 0x40000 - i; break;          // [196608, 262143]
    }

    i = (i * i) >> 16;
    u32 r;                         // 2 less terms than the other cos
    r = 0x0051F;                   // Constants multiplied by scaling:
    r = 0x040F0 - ((i * r) >> 16); // (PI/2)^6 / 720
    r = 0x13BD3 - ((i * r) >> 16); // (PI/2)^4 / 24
    r = 0x10000 - ((i * r) >> 16); // (PI/2)^2 / 2
    return neg ? -(i32)r : (i32)r;
}

static void aud_wave_channel(wave_s *w, i16 *buf, int len)
{
    // ENVELOPE SWEEPS
    envelope_s *e = &w->env;
    if (e->adsr != ADSR_NONE) {
        e->t += len;

        switch (e->adsr) {
        case ADSR_ATTACK:
            if (e->t < e->attack) {
                e->adsr = ADSR_ATTACK;
                w->vol  = (e->t * e->vol_peak) / e->attack;
                break;
            }
            e->t -= e->attack; // fallthrough
        case ADSR_DECAY:
            if (e->t < e->decay) {
                e->adsr = ADSR_DECAY;
                w->vol  = e->vol_peak + (e->t * (e->vol_sustain - e->vol_peak)) / e->decay;
                break;
            }
            e->t -= e->decay; // fallthrough
        case ADSR_SUSTAIN:
            if (e->sustain < 0) { // manually release
                e->t    = 0;
                e->adsr = ADSR_SUSTAIN;
                w->vol  = e->vol_sustain;
                break;
            }
            if (e->t < e->sustain) {
                e->adsr = ADSR_SUSTAIN;
                w->vol  = e->vol_sustain;
                break;
            }
            e->t -= e->sustain; // fallthrough
        case ADSR_RELEASE:
            if (e->t < e->release) {
                e->adsr = ADSR_RELEASE;
                w->vol  = e->vol_sustain - (e->t * e->vol_sustain) / e->release;
                break;
            }
            e->t    = 0;
            e->adsr = ADSR_NONE;
            w->incr = 0;
            return; // finished
        }
    }

    // SYNTH
    i16 *bn = buf;
    switch (w->type) {
    case WAVE_TYPE_SQUARE:
        for (int n = 0; n < len; n++, bn++, w->t += w->incr) {
            i32 b = (0x80000000U <= w->t ? w->vol : -w->vol);
            *bn   = clamp_i(*bn + b, I16_MIN, I16_MAX);
        }
        break;
        //
    case WAVE_TYPE_SINE:
        for (int n = 0; n < len; n++, bn++, w->t += w->incr) {
            i32 b = (synth_wave_sine(w->t) * w->vol) >> 16;
            *bn   = clamp_i(*bn + b, I16_MIN, I16_MAX);
        }
        break;
        //
    case WAVE_TYPE_TRIANGLE:
        for (int n = 0; n < len; n++, bn++, w->t += w->incr) {
            i32 t = w->t >> 16, b;
            if (t <= 0x4000) // 0 - 0.25
                b = (t * w->vol) >> 14;
            else if (t <= 0xC000) // 0.25 - 0.75
                b = w->vol - (((t - 0x4000) * w->vol) >> 14);
            else // 0.75 - 1
                b = (((t - 0xC000) * w->vol) >> 14) - w->vol;
            *bn = clamp_i(*bn + b, I16_MIN, I16_MAX);
        }
        break;
        //
    case WAVE_TYPE_SAW:
        for (int n = 0; n < len; n++, bn++, w->t += w->incr) {
            i32 t = w->t >> 16, b;
            if (t <= 0x8000) // 0 - 0.5
                b = (t * w->vol) >> 15;
            else // 0.5 - 1
                b = (((t - 0x8000) * w->vol) >> 15) - w->vol;
            *bn = clamp_i(*bn + b, I16_MIN, I16_MAX);
        }
        break;
        //
    case WAVE_TYPE_NOISE:
        for (int n = 0; n < len; n++, bn++) {
            i32 b = rngr_i32(-w->vol, +w->vol);
            *bn   = clamp_i(*bn + b, I16_MIN, I16_MAX);
        }
        break;
        //
    case WAVE_TYPE_SAMPLE:
        for (int n = 0; n < len; n += 2, bn += 2, w->t += w->incr) {
            int i = ((u32)(w->t >> 16) * w->sample->len) >> 16;
            i32 b = (w->sample->buf[i] * w->vol) >> 7;    // S8 -> S16
            bn[0] = clamp_i(bn[0] + b, I16_MIN, I16_MAX); // 22050 -> 44100
            bn[1] = clamp_i(bn[1] + b, I16_MIN, I16_MAX);
        }
        break;
    }
}

static void wave_envelope(wave_s *w, i32 vol_peak, i32 vol_sustain,
                          int ms_attack, int ms_decay, int ms_release)
{
    envelope_s *e  = &w->env;
    e->t           = 0;
    e->adsr        = ADSR_ATTACK;
    e->vol_peak    = vol_peak;
    e->vol_sustain = vol_sustain;
    e->attack      = (ms_attack * 44100) / 1000;
    e->decay       = (ms_decay * 44100) / 1000;
    e->release     = (ms_release * 44100) / 1000;
    e->sustain     = 0;
}

void aud_audio_cb(i16 *buf, int len)
{
    // memset(buf, 0, sizeof(i16) * len);

    wave_s *wave = &AUD.waves[0];
    wave->vol    = 10000;
    wave->type   = WAVE_TYPE_SINE;

    if (inp_debug_space() && 0) {
        if (wave->incr == 0) {
            // wave_envelope(wave, 10000, 5000, 50, 100, 100);
            aud_set_freq(wave, 200);
        }
    }

    for (int k = 0; k < AUD_WAVES; k++) {
        wave_s *w = &AUD.waves[k];
        if (w->incr == 0) continue;
        aud_wave_channel(w, buf, len);
    }
}

static wav_s wav_conv_to_i8_22050(wav_s src, alloc_s ma)
{
    wav_s r = {0};
    r.fmt   = WAV_FMT_22050_I8;

    switch (src.fmt) {
    case WAV_FMT_22050_I8:
        r.len = src.len;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i8));
        for (int i = 0; i < r.len; i++)
            ((i8 *)r.buf)[i] = ((i8 *)src.buf)[i];
        break;
    case WAV_FMT_44100_I8:
        r.len = src.len / 2;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i8));
        for (int i = 0; i < r.len; i++)
            ((i8 *)r.buf)[i] = ((i8 *)src.buf)[i << 1];
        break;
    case WAV_FMT_22050_I16:
        r.len = src.len;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i8));
        for (int i = 0; i < r.len; i++)
            ((i8 *)r.buf)[i] = ((i16 *)src.buf)[i] >> 8;
        break;
    case WAV_FMT_44100_I16:
        r.len = src.len / 2;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i8));
        for (int i = 0; i < r.len; i++)
            ((i8 *)r.buf)[i] = ((i16 *)src.buf)[i << 1] >> 8;
        break;
    }
    return r;
}

static wav_s wav_conv_to_i8_44100(wav_s src, alloc_s ma)
{
    wav_s r = {0};
    r.fmt   = WAV_FMT_44100_I8;

    switch (src.fmt) {
    case WAV_FMT_22050_I8:
        r.len = src.len * 2;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i8));
        for (int i = 0; i < r.len; i++)
            ((i8 *)r.buf)[i] = ((i8 *)src.buf)[i >> 1];
        break;
    case WAV_FMT_44100_I8:
        r.len = src.len;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i8));
        for (int i = 0; i < r.len; i++)
            ((i8 *)r.buf)[i] = ((i8 *)src.buf)[i];
        break;
    case WAV_FMT_22050_I16:
        r.len = src.len * 2;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i8));
        for (int i = 0; i < r.len; i++)
            ((i8 *)r.buf)[i] = ((i16 *)src.buf)[i >> 1] >> 8;
        break;
    case WAV_FMT_44100_I16:
        r.len = src.len;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i8));
        for (int i = 0; i < r.len; i++)
            ((i8 *)r.buf)[i] = ((i16 *)src.buf)[i] >> 8;
        break;
    }
    return r;
}

static wav_s wav_conv_to_i16_22050(wav_s src, alloc_s ma)
{
    wav_s r = {0};
    r.fmt   = WAV_FMT_22050_I16;

    switch (src.fmt) {
    case WAV_FMT_22050_I8:
        r.len = src.len;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i16));
        for (int i = 0; i < r.len; i++)
            ((i16 *)r.buf)[i] = ((i8 *)src.buf)[i] << 8;
        break;
    case WAV_FMT_44100_I8:
        r.len = src.len / 2;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i16));
        for (int i = 0; i < r.len; i++)
            ((i16 *)r.buf)[i] = ((i8 *)src.buf)[i << 1];
        break;
    case WAV_FMT_22050_I16:
        r.len = src.len;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i16));
        for (int i = 0; i < r.len; i++)
            ((i16 *)r.buf)[i] = ((i16 *)src.buf)[i];
        break;
    case WAV_FMT_44100_I16:
        r.len = src.len / 2;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i16));
        for (int i = 0; i < r.len; i++)
            ((i16 *)r.buf)[i] = ((i16 *)src.buf)[i << 1];
        break;
    }
    return r;
}

static wav_s wav_conv_to_i16_44100(wav_s src, alloc_s ma)
{
    wav_s r = {0};
    r.fmt   = WAV_FMT_44100_I16;

    switch (src.fmt) {
    case WAV_FMT_22050_I8:
        r.len = src.len * 2;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i16));
        for (int i = 0; i < r.len; i++)
            ((i16 *)r.buf)[i] = ((i8 *)src.buf)[i >> 1] << 8;
        break;
    case WAV_FMT_44100_I8:
        r.len = src.len;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i16));
        for (int i = 0; i < r.len; i++)
            ((i16 *)r.buf)[i] = ((i8 *)src.buf)[i] << 8;
        break;
    case WAV_FMT_22050_I16:
        r.len = src.len * 2;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i16));
        for (int i = 0; i < r.len; i++)
            ((i16 *)r.buf)[i] = ((i16 *)src.buf)[i >> 1];
        break;
    case WAV_FMT_44100_I16:
        r.len = src.len;
        r.buf = ma.allocf(ma.ctx, r.len * sizeof(i16));
        for (int i = 0; i < r.len; i++)
            ((i16 *)r.buf)[i] = ((i16 *)src.buf)[i];
        break;
    }
    return r;
}
