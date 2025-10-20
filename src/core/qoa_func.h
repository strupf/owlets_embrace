// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#if 0
#undef QOA_FUNC_STEREO
#define QOA_FUNC_STEREO 1
#error "qoa func editing enabled"
#endif

#include "core/qoa.h"
#include "pltf/pltf.h"
#include "pltf/pltf_intrin.h"
#include "util/mathfunc.h"

static inline i32 qoa_decode_sample(qoa_dec_s *d);
static void       qoa_decode_init_slice(qoa_dec_s *d, u64 s);

static void qoa_stream_next_slice(qoa_stream_s *q);

ATTRIBUTE_SECTION(".text.audio")
#if QOA_FUNC_STEREO
bool32 qoa_stream_stereo(qoa_stream_s *q, i16 *lbuf, i16 *rbuf, i32 len, i32 l_q16, i32 r_q16)
#else
bool32 qoa_stream_mono(qoa_stream_s *q, i16 *lbuf, i32 len, i32 l_q16)
#endif
{
    if (!q->f) return 0;
#if QOA_FUNC_STEREO
    assert(rbuf);
    i16 *rb = rbuf;
#endif
    assert(lbuf);
    i16 *lb = lbuf;
    i32  l  = len;

    if (q->pos < 0) { // delayed, actually start playing if 0<=pos
        i32 fast_forward = min_i32(l, -q->pos);
        q->pos += fast_forward;
        l -= fast_forward;
        lb += fast_forward;
#if QOA_FUNC_STEREO
        rb += fast_forward;
#endif
    }

    while (l) {
        i32 spos = q->pos % QOA_SLICE_LEN; // position in slice
        i32 n    = min3_i32(QOA_SLICE_LEN - spos, q->loop_pos_end - q->pos, l);
        l -= n;
        q->pos += n;

        // changed to pure mono only for simplicity
        // decode mono music into mono or stereo output
        for (i32 k = 0; k < n; k++) {
            i16 sample = qoa_decode_sample(&q->ds[0]);

            *lb = QOA_ADD_SAMPLE(*lb, mul_q16(l_q16, sample));
            lb++;
#if QOA_FUNC_STEREO
            *rb = QOA_ADD_SAMPLE(*rb, mul_q16(r_q16, sample));
            rb++;
#endif
        }

        if (q->pos == q->loop_pos_end) { // loop back
            if (q->repeat) {
                qoa_stream_seek(q, q->loop_pos_beg);
            } else {
                qoa_stream_end(q);
                return 0;
            }
        } else if (spos + n == QOA_SLICE_LEN) { // decode a new slice
            qoa_stream_next_slice(q);
        }
    }
    return 1;
}

void qoa_data_rewind(qoa_data_s *q);

ATTRIBUTE_SECTION(".text.audio")
#if QOA_FUNC_STEREO
bool32 qoa_data_stereo(qoa_data_s *q, i16 *lbuf, i16 *rbuf, i32 len, i32 l_q16, i32 r_q16)
#else
bool32 qoa_data_mono(qoa_data_s *q, i16 *lbuf, i32 len, i32 l_q16)
#endif
{
#if QOA_FUNC_STEREO
    assert(rbuf);
    i16 *rb = rbuf;
#endif
    assert(lbuf);
    i16 *lb = lbuf;

    for (i32 n = 0; n < len; n++) {
        i32 p = (i32)(((u32)q->pos_pitched++ * q->pitch_q8) >> 8);

        while (q->pos < p) {                        // seek and decode forward
            i32 spos      = q->pos % QOA_SLICE_LEN; // position in slice
            i32 n_samples = min_i32(QOA_SLICE_LEN - spos, p - q->pos);
            q->pos += n_samples;

            for (i32 i = 0; i < n_samples; i++) {
                q->sample = qoa_decode_sample(&q->ds);
            }

            if (spos + n_samples == QOA_SLICE_LEN) { // decode a new slice
                qoa_decode_init_slice(&q->ds, q->slices[q->pos / QOA_SLICE_LEN]);
            }
        }

        *lb = QOA_ADD_SAMPLE(*lb, mul_q16(l_q16, q->sample));
        lb++;
#if QOA_FUNC_STEREO
        *rb = QOA_ADD_SAMPLE(*rb, mul_q16(r_q16, q->sample));
        rb++;
#endif

        if (q->pos_pitched == q->len_pitched) {
            if (q->repeat) {
                qoa_data_rewind(q);
            } else {
                qoa_data_end(q);
                return 0;
            }
        }
    }
    return 1;
}