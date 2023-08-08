// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os_internal.h"

snd_s snd_get(int ID)
{
        // PD->sound->fileplayer->
        ASSERT(0 <= ID && ID < NUM_SNDID);
        return g_os.snd_tab[ID];
}

#if defined(TARGET_DESKTOP)
void os_backend_audio_init()
{
        InitAudioDevice();
}

void os_backend_audio_close()
{
        CloseAudioDevice();
}

#endif

#if defined(TARGET_PD)
// int AudioSourceFunction(void* context, i16* left, i16* right, int len); // len is # of samples in each buffer, function should return 1 if it produced output

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
        /*
        synth = PD_s_synth.newSynth();
        PD_s_synth.setWaveform(synth, kWaveformTriangle);
        PD_s_synth.playNote(synth, 100.f, 1.f, -1, 0);
        */

        // FilePlayer *fp         = PD_s_fileplayer.newPlayer();
        // PD_s_fileplayer.loadIntoPlayer(fp, "assets/audiosample.mp3");
        // PD_s_fileplayer.setVolume(fp, 1.f, 1.f);
        // PD_s_fileplayer.play(fp, 0);
}

void os_backend_audio_close()
{
}
#endif