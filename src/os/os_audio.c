// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os_audio.h"
#include "os_internal.h"

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
        u32 subchunk2ID;
        u32 subchunk2size;
} wavheader_s;

snd_s snd_put_load(int ID, const char *filename)
{
        snd_s s = snd_load_wav(filename);
        snd_put(ID, s);
        return s;
}

snd_s snd_get(int ID)
{
        ASSERT(0 <= ID && ID < NUM_SNDID);
        return g_os.snd_tab[ID];
}

static OS_FILE *i_open_wav_file(const char *filename, wavheader_s *wh)
{
        ASSERT(sizeof(wavheader_s) == 44);
        ASSERT(filename);
        OS_FILE *f = os_fopen(filename, "rb");
        ASSERT(f);
        os_fread(wh, sizeof(wavheader_s), 1, f);
        ASSERT(wh->bitspersample == 16);
        os_fseek(f, sizeof(wavheader_s), OS_SEEK_SET);
        while (wh->subchunk2ID != 0x61746164U) { // "data"
                // os_fseek(f, 4, OS_SEEK_CUR);
                os_fread(&wh->subchunk2ID, 4, 1, f);
        }
        return f;
}

snd_s snd_load_wav(const char *filename)
{
        wavheader_s wheader;
        OS_FILE    *f = i_open_wav_file(filename, &wheader);

        snd_s snd = {0};
        snd.data  = assetmem_alloc(wheader.subchunk2size);
        snd.len   = wheader.subchunk2size / sizeof(i16);

        // we don't check for errors here...
        os_fread_n(snd.data, 1, wheader.subchunk2size, f);
        os_fclose(f);
        return snd;
}

static void audio_channel_wave(audio_channel_s *ch, i16 *left, int len)
{
        i16 *buf = left;
        for (int n = 0; n < len; n++) {
                if (ch->wavepos >= ch->wavelen) {
                        ch->playback_type = PLAYBACK_TYPE_SILENT;
                        break;
                }

                u32 i = (u32)((float)ch->wavepos++ * ch->invpitch);
                ASSERT(i < ch->wavelen_og);
                i32 val = ch->wavedata[i];
                val     = *buf + ((val * ch->vol_q8) >> 8);
                *buf++  = clamp_i(val, I16_MIN, I16_MAX);
        }
}

static void audio_channel_gen(audio_channel_s *ch, i16 *left, int len)
{
        for (int n = 0; n < len; n++) {
                i32 val = 0;
                // sin wave
                // freq = 44100 Hz / "2 pi" * v
                // v = (freq * "2 pi") / 44100
                switch (ch->gentype) {
                case WAVE_TYPE_SINE: {
                        ch->genpos += ch->sinincr;
                        ch->genpos &= 0x3FFFF; // mod by "2 pi"
                        val = sin_q16(ch->genpos) >> 1;
                } break;
                case WAVE_TYPE_SQUARE: {
                        ch->genpos++;
                        ch->genpos %= ch->squarelen;
                        if ((ch->genpos << 1) < ch->squarelen) {
                                val = I16_MIN;
                        } else {
                                val = I16_MAX;
                        }
                } break;
                }
                val     = left[n] + ((val * ch->vol_q8) >> 8);
                left[n] = clamp_i(val, I16_MIN, I16_MAX);
        }
}

// update loaded music chunk if we are running out of samples
static void music_update_chunk(music_channel_s *ch, int samples_needed)
{
        int samples_chunked = OS_MUSICCHUNK_SAMPLES - ch->chunkpos;
        if (samples_needed > 0 && samples_chunked >= samples_needed) return;

        // place chunk beginning right at streampos
        os_fseek(ch->stream,
                 ch->datapos + ch->streampos * sizeof(i16),
                 OS_SEEK_SET);
        int samples_left    = ch->streamlen - ch->streampos;
        int samples_to_read = min_i(OS_MUSICCHUNK_SAMPLES, samples_left);

        // we don't check for errors here...
        os_fread_n(ch->chunk, sizeof(i16), samples_to_read, ch->stream);
        ch->chunkpos = 0;
}

static void music_channel_fillbuf(music_channel_s *ch, i16 *left, int len)
{
        i16 *chunk = &ch->chunk[ch->chunkpos];
        ch->chunkpos += len;

        if (ch->vol_q8 == 256) { // no modification necessary, just memcpy
                os_memcpy(left, chunk, sizeof(i16) * len);
                return;
        }

        i16 *buf  = left;
        int  n    = 0;
        int  len4 = len - 4;
        while (n < len4) {
                buf[0] = (chunk[0] * ch->vol_q8) >> 8;
                buf[1] = (chunk[1] * ch->vol_q8) >> 8;
                buf[2] = (chunk[2] * ch->vol_q8) >> 8;
                buf[3] = (chunk[3] * ch->vol_q8) >> 8;
                buf += 4;
                chunk += 4;
                n += 4;
        }
        while (n < len) {
                *buf++ = (*chunk++ * ch->vol_q8) >> 8;
                n++;
        }
}

static void music_channel_stream(music_channel_s *ch, i16 *left, int len)
{
        if (!ch->stream) {
                os_memclr(left, sizeof(i16) * len);
                return;
        }

        int l = min_i(len, (int)(ch->streamlen - ch->streampos));
        music_update_chunk(ch, len);
        music_channel_fillbuf(ch, left, l);

        ch->streampos += l;
        if (ch->streampos < ch->streamlen) return;

        int samples_left = len - l;
        if (ch->looping) {
                // fill remainder of buffer and restart
                ch->streampos = 0;
                music_update_chunk(ch, 0);
                music_channel_fillbuf(ch, &left[l], samples_left);
                ch->streampos = samples_left;
        } else {
                os_memclr(&left[l], samples_left * sizeof(i16));
                mus_close();
        }
}

int os_audio_cb(void *context, i16 *left, i16 *right, int len)
{
        music_channel_stream(&g_os.musicchannel, left, len);

        for (int i = 0; i < OS_NUM_AUDIO_CHANNELS; i++) {
                audio_channel_s *ch = &g_os.audiochannels[i];
                switch (ch->playback_type) {
                case PLAYBACK_TYPE_SILENT: break;
                case PLAYBACK_TYPE_GEN:
                        audio_channel_gen(ch, left, len);
                        break;
                case PLAYBACK_TYPE_WAVE:
                        audio_channel_wave(ch, left, len);
                        break;
                }
        }
        return 1;
}

void mus_play(const char *filename)
{
        wavheader_s wheader;
        OS_FILE    *f = i_open_wav_file(filename, &wheader);

        u32 num_samples_i16 = wheader.subchunk2size / sizeof(i16);

        music_channel_s *ch = &g_os.musicchannel;
        ch->datapos         = os_ftell(f);
        ch->stream          = f;
        ch->streamlen       = num_samples_i16;
        ch->streampos       = 0;
        ch->vol_q8          = 256;
        ch->looping         = 1;
        music_update_chunk(ch, 0);
}

void mus_close()
{
        music_channel_s *ch = &g_os.musicchannel;
        if (ch->stream) {
                os_fclose(ch->stream);
                ch->stream = NULL;
        }
}

void snd_put(int ID, snd_s s)
{
        g_os.snd_tab[ID] = s;
}

void snd_play_ext(snd_s s, float vol, float pitch)
{
        // channel 0 reserved for music
        for (int i = 1; i < OS_NUM_AUDIO_CHANNELS; i++) {
                audio_channel_s *ch = &g_os.audiochannels[i];
                if (ch->playback_type != PLAYBACK_TYPE_SILENT) continue;
                ch->playback_type = PLAYBACK_TYPE_WAVE;
                ch->wavedata      = s.data;
                ch->wavelen_og    = s.len;
                ch->wavelen       = (u32)((float)s.len * pitch);
                ch->invpitch      = 1.f / pitch;
                ch->wavepos       = 0;
                ch->vol_q8        = (i32)(vol * 256.f);
                break;
        }
}

void snd_play(snd_s s)
{
        snd_play_ext(s, 1.f, 1.f);
}