// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os_internal.h"

snd_s snd_get(int ID)
{
        ASSERT(0 <= ID && ID < NUM_SNDID);
        return g_os.snd_tab[ID];
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
        u32 subchunk2ID;
        u32 subchunk2size;
} wavheader_s;

static OS_FILE *i_open_wav_file(const char *filename, wavheader_s *wh)
{
        ASSERT(sizeof(wavheader_s) == 44);
        ASSERT(filename);
        OS_FILE *f = os_fopen(filename, "rb");
        ASSERT(f);
        *wh = (wavheader_s){0};
        os_fread(wh, sizeof(wavheader_s), 1, f);
        ASSERT(wh->bitspersample == 16);
        os_fseek(f, sizeof(wavheader_s), OS_SEEK_SET);
        while (wh->subchunk2ID != 0x61746164U) { // "data"
                os_fseek(f, 4, OS_SEEK_CUR);
                os_fread(&wh->subchunk2ID, 4, 1, f);
        }
        return f;
}

snd_s snd_load_wav(const char *filename)
{
        wavheader_s wheader;
        OS_FILE    *f = i_open_wav_file(filename, &wheader);

        u32 num_samples_i16 = wheader.subchunk2size / sizeof(i16);
        os_spmem_push();
        i16 *buf = os_spmem_alloc(num_samples_i16);
        os_fread(buf, sizeof(i16), num_samples_i16, f);
        os_fclose(f);

        snd_s snd = {0};
        snd.data  = assetmem_alloc(wheader.subchunk2size);
        snd.len   = num_samples_i16;
        for (int n = 0; n < (int)num_samples_i16; n++) {
                snd.data[n] = buf[n];
        }
        os_spmem_pop();
        return snd;
}

static void audio_channel_wave(audio_channel_s *ch, i16 *left, int len)
{
        for (int n = 0; n < len; n++) {
                float val = 0.f;
                if (ch->wavepos >= ch->wavelen) {
                        ch->playback_type = PLAYBACK_TYPE_SILENT;
                        break;
                }

                u32 i = (u32)((float)ch->wavepos++ * ch->invpitch);
                ASSERT(i < ch->wavelen_og);
                val     = (float)ch->wavedata[i];
                i32 l   = left[n] + (i32)(val * ch->vol);
                left[n] = CLAMP(l, I16_MIN, I16_MAX);
        }
}

static void audio_channel_gen(audio_channel_s *ch, i16 *left, int len)
{
        for (int n = 0; n < len; n++) {
                float val = 0.f;
                // sin wave
                // freq = 44100 Hz / "2 pi" * v
                // v = (freq * "2 pi") / 44100
                switch (ch->gentype) {
                case WAVE_TYPE_SINE: {
                        ch->genpos += ch->sinincr;
                        ch->genpos &= 0x3FFFF; // mod by "2 pi"
                        val = (float)(sin_q16(ch->genpos) >> 1);
                } break;
                case WAVE_TYPE_SQUARE: {
                        ch->genpos++;
                        ch->genpos %= ch->squarelen;
                        if ((ch->genpos << 1) < ch->squarelen) {
                                val = (float)I16_MIN;
                        } else {
                                val = (float)I16_MAX;
                        }
                } break;
                }
                i32 l   = left[n] + (i32)(val * ch->vol);
                left[n] = CLAMP(l, I16_MIN, I16_MAX);
        }
}

static void music_channel_stream(music_channel_s *ch, i16 *left, int len)
{
        for (int n = 0; n < len; n++) {
                if (ch->streampos >= ch->streamlen) {
                        mus_close();
                        break;
                }
                ch->streampos++;

                i16 v;
                os_fread(&v, sizeof(i16), 1, ch->stream);
                left[n] = (i32)((float)v * ch->vol);
        }
}

int os_audio_cb(void *context, i16 *left, i16 *right, int len)
{
        music_channel_s *mc = &g_os.musicchannel;
        if (mc->stream) {
                music_channel_stream(mc, left, len);
        } else {
                os_memclr(left, sizeof(i16) * len);
        }

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
        ch->stream          = f;
        ch->streamlen       = num_samples_i16;
        ch->streampos       = 0;
        ch->vol             = 1.f;
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
                ch->vol           = vol;
                break;
        }
}

void snd_play(snd_s s)
{
        snd_play_ext(s, 1.f, 1.f);
}