// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OS_INTERNAL_H
#define OS_INTERNAL_H

#include "os.h"
#include "os_audio.h"

#define TIMING_RATE      16
#define TIMING_FRAMES    64
#define OS_DESKTOP_SCALE 1
#define OS_FPS_DELTA     0.02f // 50 FPS
#define OS_DELTA_CAP     0.05f

/* RAM needed for 60s of raw music without streaming the file:
 * 44100 Hz x 60s x 2 byte = 5.3 MB
 * 22050 Hz x 60s x 2 byte = 2.7 MB
 */

enum {
        OS_SPMEM_STACK_HEIGHT = 16,
        OS_SPMEM_SIZE         = 0x100000, // 1 MB
        OS_ASSETMEM_SIZE      = 0x400000, // 4 MB
        //
        OS_FRAMEBUFFER_SIZE   = 52 * 240,
        OS_NUM_AUDIO_CHANNELS = 8,
};

typedef struct {
        char  labels[NUM_TIMING][16];
        float times[NUM_TIMING][TIMING_FRAMES];
        int   n;
} timings_s;

typedef struct {
        timings_s timings;
        i32       tick;

#if defined(TARGET_DESKTOP)
        u8          framebuffer[OS_FRAMEBUFFER_SIZE];
        Color       texpx[416 * 240];
        Texture2D   tex;
        bool32      inverted;
        rec_i32     view;
        flags32     buttons;
        flags32     buttonsp;
        AudioStream audiostream;
#elif defined(TARGET_PD)
        u8       *framebuffer;
        PDButtons buttons;
        PDButtons buttonsp;
#endif
        bool32 crankdocked;
        bool32 crankdockedp;
        i32    crank_q16;
        i32    crankp_q16;
        float  lasttime;

        tex_s         dst;
        gfx_pattern_s dstpat;

        tex_s tex_tab[NUM_TEXID];
        fnt_s fnt_tab[NUM_FNTID];
        snd_s snd_tab[NUM_SNDID];

        memarena_s      assetmem;
        ALIGNAS(4) char assetmem_raw[OS_ASSETMEM_SIZE];

        int             n_spmem;
        char           *spmemstack[OS_SPMEM_STACK_HEIGHT];
        memarena_s      spmem;
        ALIGNAS(4) char spmem_raw[OS_SPMEM_SIZE];

        music_channel_s musicchannel;
        audio_channel_s audiochannels[OS_NUM_AUDIO_CHANNELS];

} os_s;

extern os_s g_os;

void *assetmem_alloc(size_t s);

#endif