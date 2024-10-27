// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ADPCM_HH
#define ADPCM_HH

#include "pltf/pltf_types.h"

typedef struct {
    u8 *data;        // pointer to sample data (2x 4 bit samples per byte)
    u32 len;         // number of samples
    u32 len_pitched; // length of pitched buffer
    u32 data_pos;    // index in data array
    u32 pos;         // current sample index in original buffer
    u32 pos_pitched; // current sample index in pitched buffer
    u16 pitch_q8;
    u16 ipitch_q8;
    i16 hist; // current decoded i16 sample value
    i16 step_size;
    u8  nibble;
    u8  curr_byte; // current byte value of the sample
    i16 vol_q8;    // playback volume in Q8
} adpcm_s;

void adpcm_reset_to_start(adpcm_s *pcm);
void adpcm_set_pitch(adpcm_s *pcm, i32 pitch_q8);
i32  adpcm_advance_to(adpcm_s *pcm, u32 pos);
void adpcm_playback_nonpitch_silent(adpcm_s *pcm, i32 len);
void adpcm_playback(adpcm_s *pcm, i16 *lb, i16 *rb, i32 len);
void adpcm_playback_nonpitch(adpcm_s *pcm, i16 *lb, i16 *rb, i32 len);
#endif
