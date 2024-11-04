// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// A modified version of the .qoa format (qoaformat.org).
// - 1 channel (mono)
// - 2 instead of 4 samples of LMS
// - no frames, just 64 bit slices back to back

#ifndef QOA_H
#define QOA_H

#include "pltf/pltf_types.h"

#define QOA_SLICE_BUFFER 32
#define QOA_SLICE_LEN    20

enum {
    QOA_STREAM_FLAG_REPEAT   = 1 << 0,
    QOA_STREAM_FLAG_FADE_OUT = 1 << 1,
};

typedef struct {
    u32 num_samples;
    u32 num_slices;
} qoa_file_header_s;

typedef struct {
    alignas(4) i16 h[2];
    i16            w[2];
} qoa_lms_s;

typedef struct qoa_stream_s {
    u64       slice;
    qoa_lms_s lms;
    i16      *deq;
    i16       v_q8;
    u8        spos;
    u8        slices_read;
    u8        slices_polled;
    u8        flags;
    i16       v_q8_fade_s;    // v0
    i16       v_q8_fade_d;    // v1
    u32       v_fade_t_total; // duration of fade
    u32       v_fade_t;       // current time fade
    u32       pos;
    u32       num_slices;
    u32       cur_slice;
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

typedef struct qoa_data_s {
    u64       slice;
    qoa_lms_s lms;
    i16      *deq;
    i16       sample;
    u16       vol_q8;
    u8        spos;
    bool8     repeat;
    u16       ipitch_q8;
    u32       num_slices;
    u32       cur_slice;
    u64      *slices;
    u32       pos_pitched;
    u32       len_pitched;
    u32       pos;
    u32       len;

} qoa_data_s;

bool32 qoa_data_start(qoa_data_s *q, u32 n_samples, void *dat, i32 p_q8, i32 v_q8, b32 repeat);
void   qoa_data_end(qoa_data_s *q);
void   qoa_data_play(qoa_data_s *q, i16 *buf, i32 len);
bool32 qoa_data_active(qoa_data_s *q);

#endif