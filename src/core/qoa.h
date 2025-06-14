// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// A modified version of the .qoa format (qoaformat.org).
// - 2 instead of 4 samples of LMS
//
// SFX layout
// - only mono data
// - no frames, just 64 bit slices back to back
// struct qoa_file {
//     struct {
//         u32 num_samples;
//         u32 unused;
//     } file_header;
//     u64 slices[N];
// };
//
// MUS layout
// - mono or stereo interleaved
// - frames of 256 slices (512 for stereo) for seeking
// struct qoa_file {
//     struct {
//         u32 num_samples;
//         u32 unused;
//     } file_header;
//     struct {
//         struct {
//             i16 lmsdata[2][4];
//         } frame_header;
//         u64 slices[256 * n_channels]; // last one may have less
//     } frame[N];

#ifndef QOA_H
#define QOA_H

#include "pltf/pltf_types.h"
#include "wad.h"

#define QOA_FRAME_SLICES      256
#define QOA_FRAME_SLICES_MASK (QOA_FRAME_SLICES - 1)
#define QOA_SLICE_LEN         20 // num of samples in a slice
#define QOA_FRAME_SAMPLES     (QOA_FRAME_SLICES * QOA_SLICE_LEN)

static inline i32 qoa_num_slices(u32 num_samples)
{
    return (i32)((num_samples + QOA_SLICE_LEN - 1) / QOA_SLICE_LEN);
}

typedef struct {
    u32 num_samples;
    u8  num_channels; // 1 or 2
    u8  unused[3];
} qoa_file_header_s;

// MUSIC
// unpitched streaming of qoa data from file
typedef struct { // 8 + 4 + 4 + 4 = 20
    ALIGNAS(4)
    i16 h[2]; // qoa predictor values
    i16 w[2];
} qoa_lms_s;

typedef struct qoa_dec_s {
    u64       s;   // 8
    qoa_lms_s lms; // 8
    i16      *deq; // 4
} qoa_dec_s;

// MUSIC
// unpitched streaming of qoa data from file
typedef struct qoa_mus_s {
    ALIGNAS(32)
    qoa_dec_s ds[2];
    void     *f;    // wad handle opened at startup of app
    u32       seek; // pos in file + highest bit set if stereo
    u32       num_samples;
    u32       cur_slice; // current slice index
    u32       loop_s1;
    u32       loop_s2;
    u32       pos;
    u64       slices[QOA_FRAME_SLICES * 2]; // buffer of slices, channels interleaved
} qoa_mus_s;

bool32 qoa_mus_start(qoa_mus_s *q, void *f);
void   qoa_mus_set_loop(qoa_mus_s *q, u32 s1, u32 s2);
void   qoa_mus_end(qoa_mus_s *q);
void   qoa_mus(qoa_mus_s *q, i16 *lbuf, i16 *rbuf, i32 len, i32 v_q16);
void   qoa_mus_seek_ms(qoa_mus_s *q, u32 ms);
void   qoa_mus_seek(qoa_mus_s *q, u32 pos);
bool32 qoa_mus_active(qoa_mus_s *q);

// SFX
// data is already loaded into memory
// can be pitched
typedef struct qoa_sfx_s {
    qoa_dec_s ds;
    u64      *slices;      // slice array in memory
    u32       num_slices;  // total number of slices
    u32       cur_slice;   // current slice index
    u32       pos_pitched; // pos in samples in pitched length
    u32       len_pitched; // length in samples pitched
    u32       pos;         // pos in samples in unpitched length
    u32       len;         // unpitched length in samples
    u16       ipitch_q8;   // inverse pitch in Q8
    i16       sample;      // last decoded sample
    i16       v_q8;        // volume in Q8
    i16       pan_q8;      // -256 = left only, 0 = center, +256 right only
    u8        spos;        // currently decoded sample in slice [0,19]
    bool8     repeat;
} qoa_sfx_s;

bool32 qoa_sfx_start(qoa_sfx_s *q, u32 n_samples, void *dat, i32 p_q8, i32 v_q8, b32 repeat);
void   qoa_sfx_end(qoa_sfx_s *q);
b32    qoa_sfx_play(qoa_sfx_s *q, i16 *lbuf, i16 *rbuf, i32 len, i32 v_q8);
bool32 qoa_sfx_active(qoa_sfx_s *q);

#endif