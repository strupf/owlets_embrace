// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// https://qoaformat.org/

#include "qoa.h"
#include "pltf/pltf.h"
#include "pltf/pltf_intrin.h"
#include "util/mathfunc.h"

static const i16 qoa_deq[16][8] = {
    {1, -1, 3, -3, 5, -5, 7, -7},
    {5, -5, 18, -18, 32, -32, 49, -49},
    {16, -16, 53, -53, 95, -95, 147, -147},
    {34, -34, 113, -113, 203, -203, 315, -315},
    {63, -63, 210, -210, 378, -378, 588, -588},
    {104, -104, 345, -345, 621, -621, 966, -966},
    {158, -158, 528, -528, 950, -950, 1477, -1477},
    {228, -228, 760, -760, 1368, -1368, 2128, -2128},
    {316, -316, 1053, -1053, 1895, -1895, 2947, -2947},
    {422, -422, 1405, -1405, 2529, -2529, 3934, -3934},
    {548, -548, 1828, -1828, 3290, -3290, 5117, -5117},
    {696, -696, 2320, -2320, 4176, -4176, 6496, -6496},
    {868, -868, 2893, -2893, 5207, -5207, 8099, -8099},
    {1064, -1064, 3548, -3548, 6386, -6386, 9933, -9933},
    {1286, -1286, 4288, -4288, 7718, -7718, 12005, -12005},
    {1536, -1536, 5120, -5120, 9216, -9216, 14336, -14336},
};

static inline void qoa_lms_init(qoa_lms_s *lms)
{
    lms->h[0] = 0;
    lms->h[1] = 0;
    lms->w[0] = -(1 << 13);
    lms->w[1] = +(2 << 13);
}

static inline i16 *qoa_decode_sf_tab(u64 *s)
{
    i16 *deq = (i16 *)qoa_deq[*s >> 60];
    *s <<= 4;
    return deq;
}

static inline i32 qoa_decode_sample(qoa_lms_s *lms, u64 *s, i16 *deq)
{
    i32 dq = deq[*s >> 61];
    *s <<= 3;
    i32 pr = i16x2_dot(i16x2_ld(&lms->w[0]), i16x2_ld(&lms->h[0])) >> 13;
    i32 sp = ssat(pr + dq, 16);
    i32 dt = dq >> 4;
    lms->w[0] += lms->h[0] < 0 ? -dt : +dt;
    lms->w[1] += lms->h[1] < 0 ? -dt : +dt;
    lms->h[0] = lms->h[1];
    lms->h[1] = sp;
    return sp;
}

void qoa_stream_next_slice(qoa_stream_s *q)
{
    if (q->slices_polled == q->slices_read) {
        u32 left = q->num_slices - q->cur_slice;
        u32 read = min_u32(left, QOA_SLICE_BUFFER);
        pltf_file_r(q->f, q->slices, sizeof(u64) * read);
        q->slices_read   = read;
        q->slices_polled = 0;
    }
    q->slice = q->slices[q->slices_polled++];
    q->deq   = qoa_decode_sf_tab(&q->slice);
}

void qoa_stream_rewind(qoa_stream_s *q)
{
    pltf_file_seek_set(q->f, sizeof(qoa_file_header_s));
    q->cur_slice     = 0;
    q->spos          = 0;
    q->slices_polled = 0;
    q->slices_read   = 0;
    q->pos           = 0;
    qoa_lms_init(&q->lms);
}

bool32 qoa_stream_start(qoa_stream_s *q, const char *fname, u32 ms, i32 v_q8)
{
    void *f = pltf_file_open_r(fname);
    if (!f) return 0;

    q->f     = f;
    q->v_q8  = 0;
    q->flags = QOA_STREAM_FLAG_REPEAT;
    qoa_stream_fade_to_vol(q, ms, v_q8);
    qoa_file_header_s head = {0};
    pltf_file_r(q->f, &head, sizeof(qoa_file_header_s));
    q->num_slices = qoa_num_slices(head.num_samples);
    qoa_stream_rewind(q);
    qoa_stream_next_slice(q);
    return 1;
}

void qoa_stream_end(qoa_stream_s *q)
{
    if (q->f) {
        pltf_file_close(q->f);
        q->f = 0;
    }
}

void qoa_stream(qoa_stream_s *q, i16 *buf, i32 len)
{
    qoa_lms_s *lms = &q->lms;
    i16       *b   = buf;
    u32        l   = (u32)len;
    q->pos += len;

    i32 vol = (i32)q->v_q8 << 8;
    while (l) {
        u32 n_samples = min_u32(QOA_SLICE_LEN - q->spos, l);
        l -= n_samples;
        q->spos += n_samples;

        for (u32 n = 0; n < n_samples; n++) {
            i32 s = qoa_decode_sample(lms, &q->slice, q->deq);
            i32 v = i16_adds((i32)*b, mul_q16(vol, s));
            *b++  = v;
        }

        if (q->spos == QOA_SLICE_LEN) { // decode a new slice
            q->spos = 0;
            q->cur_slice++;

            if (q->cur_slice == q->num_slices) { // new frame
                if (!(q->flags & QOA_STREAM_FLAG_REPEAT)) {
                    qoa_stream_end(q);
                    break;
                }
                qoa_stream_rewind(q);
            }
            qoa_stream_next_slice(q);
        }
    }

    if (q->v_fade_t_total) {
        q->v_fade_t += len;
        if (q->v_fade_t < q->v_fade_t_total) {
            i32 d   = ((q->v_q8_fade_d - q->v_q8_fade_s) * q->v_fade_t) / q->v_fade_t_total;
            q->v_q8 = q->v_q8_fade_s + d;
        } else {
            q->v_q8           = q->v_q8_fade_s + q->v_q8_fade_d;
            q->v_fade_t_total = 0;
            if (q->flags & QOA_STREAM_FLAG_FADE_OUT) {
                qoa_stream_end(q);
            }
        }
    }
}

bool32 qoa_stream_active(qoa_stream_s *q)
{
    return (q->f != 0);
}

void qoa_stream_fade_to_vol(qoa_stream_s *q, u32 ms, i32 v_q8)
{
    if (v_q8 < 0) {
        q->flags |= QOA_STREAM_FLAG_FADE_OUT;
    }
    if (ms == 0) {
        q->v_q8 = max_i32(v_q8, 0);
    } else {
        q->v_q8_fade_s = q->v_q8;
        q->v_q8_fade_d = max_i32(v_q8, 0);
    }
    q->v_fade_t       = 0;
    q->v_fade_t_total = (ms * 44100) / 1000;
}

void qoa_stream_seek(qoa_stream_s *q, u32 p)
{
    if (!qoa_stream_active(q)) return;
    if ((i32)q->num_slices <= qoa_num_slices(p)) return;

    qoa_stream_rewind(q);
    qoa_lms_s *lms = &q->lms;
    u32        l   = p;

    while (QOA_SLICE_LEN <= l) {
        l -= QOA_SLICE_LEN;
        for (u32 n = 0; n < QOA_SLICE_LEN; n++) {
            qoa_decode_sample(lms, &q->slice, q->deq);
        }
        q->cur_slice++;
        qoa_stream_next_slice(q);
    }

    for (u32 n = 0; n < l; n++) {
        qoa_decode_sample(lms, &q->slice, q->deq);
    }

    q->spos = l;
    q->pos  = p;
}

bool32 qoa_data_start(qoa_data_s *q, u32 n_samples, void *dat, i32 p_q8, i32 v_q8, b32 repeat)
{
    if (!q || !n_samples || !dat || p_q8 <= 0) return 0;
    assert(((uptr)dat & 7) == 0); // 8 byte alignment
    q->slices      = (u64 *)dat;
    q->num_slices  = qoa_num_slices(n_samples);
    q->len         = n_samples;
    q->len_pitched = (n_samples * p_q8) >> 8;
    q->ipitch_q8   = 65536 / p_q8;
    q->vol_q8      = v_q8;
    q->slice       = q->slices[0];
    q->deq         = qoa_decode_sf_tab(&q->slice);
    q->cur_slice   = 0;
    q->spos        = 0;
    q->pos_pitched = 0;
    q->pos         = 0;
    qoa_lms_init(&q->lms);
    assert((((q->len_pitched - 1) * q->ipitch_q8) >> 8) < n_samples);
    return 1;
}

void qoa_data_end(qoa_data_s *q)
{
    q->slices = NULL;
}

void qoa_data_play(qoa_data_s *q, i16 *buf, i32 len)
{
    qoa_lms_s *lms = &q->lms;
    i16       *b   = buf;

    i32 vol_q16 = (i32)q->vol_q8 << 8;
    for (i32 n = 0; n < len; n++) {
        u32 p = (q->pos_pitched++ * q->ipitch_q8) >> 8;
        assert(p <= q->len);

        while (q->pos < p) { // seek and decode forward
            u32 n_samples = min_u32(QOA_SLICE_LEN - q->spos, p - q->pos);
            q->pos += n_samples;
            q->spos += n_samples;

            for (u32 n = 0; n < n_samples; n++) {
                q->sample = qoa_decode_sample(lms, &q->slice, q->deq);
            }

            if (q->spos == QOA_SLICE_LEN) { // decode a new slice
                q->spos = 0;
                q->cur_slice++;
                q->slice = q->slices[q->cur_slice];
                q->deq   = qoa_decode_sf_tab(&q->slice);
            }
        }

        i32 v = i16_adds((i32)*b, mul_q16(vol_q16, q->sample));
        *b++  = v;

        if (q->pos_pitched == q->len_pitched) {
            qoa_data_end(q);
            break;
        }
    }
}

bool32 qoa_data_active(qoa_data_s *q)
{
    return (q->slices != 0);
}