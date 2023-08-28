// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef OS_AUDIO_H
#define OS_AUDIO_H

#include "os_fileio.h"
#include "os_types.h"

enum {
        OS_MUSICCHUNK         = 0x40000, // 256 KB
        OS_MUSICCHUNK_SAMPLES = OS_MUSICCHUNK / sizeof(i16),
};

enum {
        PLAYBACK_TYPE_SILENT,
        PLAYBACK_TYPE_WAVE,
        PLAYBACK_TYPE_GEN,
};

typedef struct {
        int   playback_type;
        i32   vol_q8;
        float invpitch; // 1 / pitch

        union {
                struct {
                        i16 *wavedata;
                        u32  wavelen;
                        u32  wavelen_og;
                        u32  wavepos;
                };
                struct {
                        int gentype;
                        u32 genfreq;
                        i32 genpos;
                        i32 sinincr; // = (freq * "2 pi") / 44100
                        i32 squarelen;
                };
        };
} audio_channel_s;

typedef struct {
        OS_FILE *stream;
        u32      datapos;
        u32      streampos; // position in samples
        u32      streamlen;
        i32      vol_q8;
        float    invpitch; // 1 / pitch
        bool32   looping;

        i16 chunk[OS_MUSICCHUNK_SAMPLES];
        int chunkpos; // position in samples in chunk
} music_channel_s;

int os_audio_cb(void *context, i16 *left, i16 *right, int len);

#endif