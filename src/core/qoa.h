// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// A modified version of the .qoa format (qoaformat.org).
// - 1 channel (mono)
// - 2 instead of 4 samples of LMS
// - no frames, just 64 bit slices back to back
//
// struct qoa_file {
//    struct {
//        u32 num_samples;
//        u32 unused;
//    } file_header;
//    u64 slices[N];
// };

#ifndef QOA_H
#define QOA_H

#include "pltf/pltf_types.h"
#include "wad.h"

#define QOA_SLICE_LEN           20 // num of samples in a slice
#define QOA_MUS_MAX_CHANNELS    2
#define QOA_MUS_SLICES_BUFFERED 32
#define QOA_SLICE_LEN           20                                               // num of samples in a slice
#define QOA_SLICE_BUFFER        (QOA_MUS_MAX_CHANNELS * QOA_MUS_SLICES_BUFFERED) // how many slices to buffer in stream

typedef struct {
    u32 num_samples;
    u8  num_channels; // 1 or 2
    u8  unused[3];
} qoa_file_header_s;

static inline i32 qoa_num_slices(u32 num_samples)
{
    return (i32)((num_samples + QOA_SLICE_LEN - 1) / QOA_SLICE_LEN);
}

// qoa predictor values
typedef struct {
    ALIGNAS(4)
    i16 h[2];
    i16 w[2];
} qoa_lms_s;

enum {
    QOA_STREAM_FLAG_REPEAT   = 1 << 0,
    QOA_STREAM_FLAG_FADE_OUT = 1 << 1,
};

enum {
    QOA_MUS_MODE_MO_MO,
    QOA_MUS_MODE_MO_ST,
    QOA_MUS_MODE_ST_MO,
    QOA_MUS_MODE_ST_ST,
};

// MUSIC
// unpitched streaming of qoa data from file
typedef struct { // 8 + 4 + 4 + 4 = 20
    u64  s;
    i16  h[2];
    i16  w[2];
    i16 *deq;
} qoa_dec_s;

// MUSIC
// unpitched streaming of qoa data from file
typedef struct qoa_mus_s {
    qoa_dec_s ds[QOA_MUS_MAX_CHANNELS];
    i16       v_q8;          // volume in Q8
    u8        spos;          // currently decoded sample in slice [0,19]
    u8        slices_read;   // slices buffered
    u8        slices_polled; // slices polled out of buffered slices
    u8        flags;         //
    u8        n_channels;
    u32       pos;                      // sample position in unpitched data
    u32       num_slices;               // total number of slices across all channels
    u32       cur_slice;                // current slice index
    void     *f;                        // wad handle opened at startup of app
    u32       seek;                     // not active if zero!
    u64       slices[QOA_SLICE_BUFFER]; // buffer of slices, channels interleaved
} qoa_mus_s;

bool32 qoa_mus_start(qoa_mus_s *q, void *f);
void   qoa_mus_end(qoa_mus_s *q);
void   qoa_mus(qoa_mus_s *q, i16 *lbuf, i16 *rbuf, i32 len);
bool32 qoa_mus_active(qoa_mus_s *q);
void   qoa_mus_set_vol(qoa_mus_s *q, i32 v);

// SFX
// data is already loaded into memory
// can be pitched
typedef struct qoa_sfx_s {
    qoa_dec_s ds;
    i16       sample;      // last decoded sample
    u16       vol_q8;      // volume in Q8
    u8        spos;        // currently decoded sample in slice [0,19]
    u16       ipitch_q8;   // inverse pitch in Q8
    u32       num_slices;  // total number of slices
    u32       cur_slice;   // current slice index
    i32       pan_q8;      // -256 = left only, 0 = center, +256 right only
    u64      *slices;      // slice array in memory
    u32       pos_pitched; // pos in samples in pitched length
    u32       len_pitched; // length in samples pitched
    u32       pos;         // pos in samples in unpitched length
    u32       len;         // unpitched length in samples
} qoa_sfx_s;

bool32 qoa_sfx_start(qoa_sfx_s *q, u32 n_samples, void *dat, i32 p_q8, i32 v_q8, b32 repeat);
void   qoa_sfx_end(qoa_sfx_s *q);
void   qoa_sfx_play(qoa_sfx_s *q, i16 *lbuf, i16 *rbuf, i32 len);
bool32 qoa_sfx_active(qoa_sfx_s *q);

#endif