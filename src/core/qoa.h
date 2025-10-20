// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// A modified version of the .qoa format (qoaformat.org).
// 44100 Hz
// - 2 instead of 4 samples of LMS
//
// SFX layout
// - only mono data
// - no frames, just 64 bit slices back to back
// struct qoa_file {
//     struct {
//         u32 num_samples;
//         u8  n_channels;
//         u8  unused[3];
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
//         u8  n_channels;
//         u8  unused[3];
//     } file_header;
//     struct {
//         struct {
//             i16 lmsdata[2][4];
//         } frame_header;
//         u64 slices[256 * n_channels]; // last one is zero padded
//     } frame[N];

#ifndef QOA_H
#define QOA_H

#include "pltf/pltf_types.h"
#include "wad.h"

#if 1 // use prefetching
#define QOA_PREFETCH PREFETCH
#endif

#if 0 // whether audio samples should be added to the buffer using saturating addition
#define QOA_ADD_SAMPLE(A, B) i16_adds((i32)(A), (i32)(B))
#else
#define QOA_ADD_SAMPLE(A, B) (i16)((i32)(A) + (i32)(B))
#endif

#define QOA_FRAME_SLICES           256
#define QOA_SLICE_LEN              20 // num of samples in a slice
#define QOA_STREAM_SLICES_BUFFERED 64
#define QOA_FRAME_SLICES_BUF_MASK  (QOA_STREAM_SLICES_BUFFERED - 1)
#define QOA_FRAME_SLICES_MASK      (QOA_FRAME_SLICES - 1)
#define QOA_FRAME_SAMPLES          (QOA_FRAME_SLICES * QOA_SLICE_LEN)

static_assert(IS_POW2(QOA_FRAME_SLICES), "num frame slices must be pow2");
static_assert(IS_POW2(QOA_STREAM_SLICES_BUFFERED), "num buffered slices must be pow2");
static_assert((QOA_FRAME_SLICES % QOA_STREAM_SLICES_BUFFERED) == 0,
              "num of frame slices must be multiple num of buffered slices");

static inline i32 qoa_num_slices(u32 num_samples)
{
    return (i32)((num_samples + QOA_SLICE_LEN - 1) / QOA_SLICE_LEN);
}

typedef struct {
    ALIGNAS(8)
    u32 num_samples;
    u8  num_channels; // 1 or 2
    u8  unused[3];
} qoa_file_header_s;

// qoa predictor values
typedef struct {
    ALIGNAS(8)
    i16 h[2];
    i16 w[2];
} qoa_lms_s;

// qoa slice decoding struct neatly fits a cache line
// slice, predictor values and step table entry
typedef struct qoa_dec_s {
    ALIGNAS(32)
    u64       s;       // 8
    qoa_lms_s lms;     // 8
    i16       deqt[8]; // 16, copy of sample delta table entry for this slice
} qoa_dec_s;

// play unpitched stream of qoa data from file
// kept simple -> handling of volume settings is done by the caller
typedef struct qoa_stream_s {
    // cache lines
    ALIGNAS(32)
    qoa_dec_s ds[2]; // for 2 channels

    // cache line
    ALIGNAS(32)
    void *f;            // 4 - 4  wad handle opened at startup of app (32 bit on PD)
    i32   loop_pos_beg; // 4 - 8 position in samples
    i32   loop_pos_end; // 4 - 12 position in samples
    i32   pos;          // 4 - 16 position in samples, negative if delayed playback
    i32   num_samples;  // 4 - 20
    i32   seek;         // 4 - 24 only for rewinding
    b8    repeat;       // 1 - 25

    // cache line
    ALIGNAS(32)
    u64 slices[QOA_STREAM_SLICES_BUFFERED]; // buffer of slices, channels interleaved
} qoa_stream_s;

bool32 qoa_stream_start(qoa_stream_s *q, void *f, i32 pos_beg, i32 loop_pos_beg, i32 loop_pos_end, b32 repeat);
void   qoa_stream_set_loop(qoa_stream_s *q, i32 loop_pos_beg, i32 loop_pos_end);
void   qoa_stream_end(qoa_stream_s *q);
void   qoa_stream_seek(qoa_stream_s *q, i32 sample_pos);
bool32 qoa_stream_active(qoa_stream_s *q);
bool32 qoa_stream_stereo(qoa_stream_s *q, i16 *lbuf, i16 *rbuf, i32 len, i32 l_q16, i32 r_q16);
bool32 qoa_stream_mono(qoa_stream_s *q, i16 *lbuf, i32 len, i32 l_q16);

// play qoa from ram; no frames and mono source only; can be pitched
// kept simple -> handling of volume settings and panning is done by the caller
typedef struct qoa_data_s {
    // cache line
    ALIGNAS(32)
    qoa_dec_s ds;

    // cache line
    ALIGNAS(32)
    u64  *slices;      // 4 - 4  slice array in memory
    i32   pos_pitched; // 4 - 8  pos in samples in pitched length
    i32   len_pitched; // 4 - 12 length in samples pitched
    i32   pos;         // 4 - 16 pos in samples in unpitched length
    u16   pitch_q8;    // 2 - 18 pitch in Q8
    i16   sample;      // 2 - 20 last decoded sample
    bool8 repeat;      // 1 - 21
} qoa_data_s;

bool32 qoa_data_start(qoa_data_s *q, void *data, i32 pitch_q8, i32 pan_q8, b32 repeat);
void   qoa_data_end(qoa_data_s *q);
bool32 qoa_data_active(qoa_data_s *q);
bool32 qoa_data_stereo(qoa_data_s *q, i16 *lbuf, i16 *rbuf, i32 len, i32 l_q16, i32 r_q16);
bool32 qoa_data_mono(qoa_data_s *q, i16 *lbuf, i32 len, i32 l_q16);

#endif