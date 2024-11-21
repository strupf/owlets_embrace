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
//        u32 num_slices;
//    } file_header;
//    u64 slices[N];
// };

#ifndef QOA_H
#define QOA_H

#include "pltf/pltf_types.h"

#define QOA_SLICE_LEN 20 // num of samples in a slice

typedef struct {
    u32  num_samples;
    byte unused[4];
} qoa_file_header_s;

static inline i32 qoa_num_slices(u32 num_samples)
{
    return (i32)((num_samples + QOA_SLICE_LEN - 1) / QOA_SLICE_LEN);
}

// qoa predictor values
typedef struct {
    alignas(4) i16 h[2];
    i16            w[2];
} qoa_lms_s;

#define QOA_SLICE_BUFFER 32 // how many slices to buffer in stream

enum {
    QOA_STREAM_FLAG_REPEAT   = 1 << 0,
    QOA_STREAM_FLAG_FADE_OUT = 1 << 1,
};

// MUSIC
// unpitched streaming of qoa data from file
typedef struct {
    u64       slice;          // currently decoded slice
    qoa_lms_s lms;            //
    i16      *deq;            // pointer to dequantization table of cur slice
    i16       v_q8;           // volume in Q8
    u8        spos;           // currently decoded sample in slice [0,19]
    u8        slices_read;    // slices buffered
    u8        slices_polled;  // slices polled out of buffered slices
    u8        flags;          //
    i16       v_q8_fade_s;    // v0
    i16       v_q8_fade_d;    // v1
    u32       v_fade_t_total; // duration of fade
    u32       v_fade_t;       // current time fade
    u32       pos;            // sample position in unpitched data
    u32       num_slices;     // total number of slices
    u32       cur_slice;      // current slice index
    void     *f;
    u64       slices[QOA_SLICE_BUFFER];
} qoa_stream_s;

bool32 qoa_stream_start(qoa_stream_s *q, const char *fname, u32 ms, i32 v_q8);
void   qoa_stream_end(qoa_stream_s *q);
void   qoa_stream_seek(qoa_stream_s *q, u32 p);
void   qoa_stream(qoa_stream_s *q, i16 *buf, i32 len);
bool32 qoa_stream_active(qoa_stream_s *q);

// v_q8 < 0 -> fade out and stop
void qoa_stream_fade_to_vol(qoa_stream_s *q, u32 ms, i32 v_q8);

// SFX
// data is already loaded into memory
// can be pitched
typedef struct qoa_data_s {
    u64       slice;       // currently decoded slice
    qoa_lms_s lms;         //
    i16      *deq;         // pointer to dequantization table of cur slice
    i16       sample;      // last decoded sample
    u16       vol_q8;      // volume in Q8
    u8        spos;        // currently decoded sample in slice [0,19]
    u16       ipitch_q8;   // inverse pitch in Q8
    u32       num_slices;  // total number of slices
    u32       cur_slice;   // current slice index
    u64      *slices;      // slice array in memory
    u32       pos_pitched; // pos in samples in pitched length
    u32       len_pitched; // length in samples pitched
    u32       pos;         // pos in samples in unpitched length
    u32       len;         // unpitched length in samples
} qoa_data_s;

bool32 qoa_data_start(qoa_data_s *q, u32 n_samples, void *dat, i32 p_q8, i32 v_q8, b32 repeat);
void   qoa_data_end(qoa_data_s *q);
void   qoa_data_play(qoa_data_s *q, i16 *buf, i32 len);
bool32 qoa_data_active(qoa_data_s *q);

#endif