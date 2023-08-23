// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os_internal.h"

snd_s snd_get(int ID)
{
        ASSERT(0 <= ID && ID < NUM_SNDID);
        return g_os.snd_tab[ID];
}

static int os_audio_cb(void *context, i16 *left, i16 *right, int len)
{
        os_memclr(left, sizeof(i16) * len);

        for (int i = 0; i < OS_NUM_AUDIO_CHANNELS; i++) {
                audio_channel_s *ch = &g_os.audiochannels[i];
                if (!ch->active) continue;

                for (int n = 0; n < len; n++) {
                        // sin wave
                        ch->state += 1024;
                        i32 l = left[n];
                        l += (sin_q16(ch->state) >> 1);
                        left[n] = CLAMP(l, I16_MIN, I16_MAX);
                }
        }
        return 1;
}

#if defined(TARGET_DESKTOP)

void audio_callback(void *buf, uint len)
{
        static i16 audiomem_local[0x4000];

        if (os_audio_cb(NULL, audiomem_local, NULL, len)) {
                os_memcpy(buf, audiomem_local, sizeof(i16) * len);
        } else {
                os_memclr(buf, sizeof(i16) * len);
        }
}

void os_backend_audio_init()
{
        InitAudioDevice();
        g_os.audiochannels[0].active = 1;

        g_os.audiostream = LoadAudioStream(44100, 16, 1);
        SetAudioStreamCallback(g_os.audiostream, audio_callback);
        PlayAudioStream(g_os.audiostream);
}

void os_backend_audio_close()
{
        UnloadAudioStream(g_os.audiostream);
        CloseAudioDevice();
}

#endif

#if defined(TARGET_PD)

struct playdate_sound_fileplayer PD_s_fileplayer;
struct playdate_sound_channel    PD_s_channel;
struct playdate_sound_synth      PD_s_synth;

u32 (*PD_s_getCurrentTime)(void);
SoundSource *(*PD_s_addSource)(AudioSourceFunction *callback, void *context, int stereo);
SoundChannel *(*PD_s_getDefaultChannel)(void);
int (*PD_s_addChannel)(SoundChannel *channel);
int (*PD_s_removeChannel)(SoundChannel *channel);
int (*PD_s_removeSource)(SoundSource *source);

PDSynth *synth;

void os_backend_audio_init()
{
        PD_s_getCurrentTime    = PD->sound->getCurrentTime;
        PD_s_addSource         = PD->sound->addSource;
        PD_s_getDefaultChannel = PD->sound->getDefaultChannel;
        PD_s_addChannel        = PD->sound->addChannel;
        PD_s_removeChannel     = PD->sound->removeChannel;
        PD_s_removeSource      = PD->sound->removeSource;
        PD_s_fileplayer        = *PD->sound->fileplayer;
        PD_s_channel           = *PD->sound->channel;
        PD_s_synth             = *PD->sound->synth;

        PD->sound->addSource(os_audio_cb, NULL, 0);
        g_os.audiochannels[0].active = 1;
}

void os_backend_audio_close()
{
}
#endif