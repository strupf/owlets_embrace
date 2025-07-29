// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// https://qoaformat.org/

#include "qoa.h"
#include "pltf/pltf.h"
#include "pltf/pltf_intrin.h"
#include "util/mathfunc.h"

ALIGNAS(32)
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

enum {
    QOA_MUS_MODE_MO_ST,
    QOA_MUS_MODE_ST_ST
};

typedef struct {
    qoa_lms_s lms[2];
} qoa_frameheader_s;

static void qoa_decode_init(qoa_dec_s *d)
{
    d->lms.h[0] = 0;
    d->lms.h[1] = 0;
    d->lms.w[0] = -(1 << 13);
    d->lms.w[1] = +(2 << 13);
}

static void qoa_decode_init_slice(qoa_dec_s *d, u64 s)
{
    mcpy(d->deqt, qoa_deq[s >> 60], sizeof(i16) * 8);
    d->s = s << 4;
}

static inline i32 qoa_decode_sample(qoa_dec_s *d)
{
    i32 dq = d->deqt[d->s >> 61];
    d->s <<= 3;
    i32 pr = i16x2_dot(i16x2_ld(&d->lms.w[0]), i16x2_ld(&d->lms.h[0])) >> 13;
#if 0 // needs clamp? -> see output of audio converter
    i32 sp = ssat(pr + dq, 16);
#else
    i32 sp = pr + dq;
#endif
    i32 dt = dq >> 4;
    d->lms.w[0] += d->lms.h[0] < 0 ? -dt : +dt;
    d->lms.w[1] += d->lms.h[1] < 0 ? -dt : +dt;
    d->lms.h[0] = d->lms.h[1];
    d->lms.h[1] = sp;
    return sp;
}

void qoa_mus_next_slice(qoa_mus_s *q)
{
    i32 stereo = q->seek >> 31;
    for (i32 n = 0; n <= stereo; n++) {
        i32 sl = ((q->cur_slice & QOA_FRAME_SLICES_MASK) << stereo) + n;
        qoa_decode_init_slice(&q->ds[n], q->slices[sl]);
    }
}

void qoa_mus_next_frame(qoa_mus_s *q)
{
    qoa_frameheader_s h = {0};
    pltf_file_r(q->f, &h, sizeof(qoa_frameheader_s));
    q->ds[0].lms = h.lms[0];
    q->ds[1].lms = h.lms[1];
    i32 n        = min_u32(qoa_num_slices(q->num_samples) - q->cur_slice, QOA_FRAME_SLICES);
    i32 stereo   = q->seek >> 31;
    pltf_file_r(q->f, q->slices, (sizeof(u64) * n) << stereo);
}

void qoa_mus_rewind(qoa_mus_s *q)
{
    i32 seek = q->seek & ~((u32)1 << 31);
    pltf_file_seek_set(q->f, seek + sizeof(qoa_file_header_s));
    q->cur_slice = 0;
    q->pos       = 0;
    q->spos      = 0;
    qoa_decode_init(&q->ds[0]);
    qoa_decode_init(&q->ds[1]);
}

bool32 qoa_mus_start(qoa_mus_s *q, void *f)
{
    if (!f) return 0;

    mclr(q, sizeof(qoa_mus_s));

    qoa_file_header_s h    = {0};
    i32               seek = pltf_file_tell(f);
    pltf_file_seek_set(f, seek);
    pltf_file_r(f, &h, sizeof(qoa_file_header_s));
    q->f           = f;
    q->seek        = seek | (h.num_channels == 2 ? (u32)1 << 31 : 0);
    q->num_samples = h.num_samples;
    q->loop_s1     = 0;
    q->loop_s2     = h.num_samples;
    qoa_mus_rewind(q);
    qoa_mus_next_frame(q);
    qoa_mus_next_slice(q);
    return 1;
}

void qoa_mus_set_loop(qoa_mus_s *q, u32 s1, u32 s2)
{
    q->loop_s1 = s1;
    q->loop_s2 = s2 ? min_u32(s2, q->num_samples) : q->num_samples;
}

void qoa_mus_end(qoa_mus_s *q)
{
    if (q->f) {
        pltf_file_close(q->f);
    }
    mclr(q, sizeof(qoa_mus_s));
}

void qoa_mus(qoa_mus_s *q, i16 *lbuf, i16 *rbuf, i32 len, i32 v_q16)
{
    if (!q->f) return;

    i16 *br   = rbuf;
    i16 *bl   = lbuf;
    i32  mode = q->seek >> 31; // stereo (1) or mono (0) file
    u32  l    = (u32)len;

    while (l) {
        u32 n = min3_u32(QOA_SLICE_LEN - q->spos, q->loop_s2 - q->pos, l);
        l -= n;
        q->pos += n;
        q->spos += n;

        switch (mode) {
        case QOA_MUS_MODE_MO_ST:
            // decode mono music into stereo output
            for (u32 k = 0; k < n; k++, bl++, br++) {
                q->sample_l = mul_q16(v_q16, qoa_decode_sample(&q->ds[0]));
                *bl         = i16_adds((i32)*bl, q->sample_l);
                *br         = i16_adds((i32)*br, q->sample_l);
            }
            break;
        case QOA_MUS_MODE_ST_ST:
            // decode stereo music into stereo output
            for (u32 k = 0; k < n; k++, bl++, br++) {
                q->sample_l = mul_q16(v_q16, qoa_decode_sample(&q->ds[0]));
                q->sample_r = mul_q16(v_q16, qoa_decode_sample(&q->ds[1]));
                *br         = i16_adds((i32)*br, q->sample_r);
                *bl         = i16_adds((i32)*bl, q->sample_l);
            }
            break;
        }

        if (q->pos == q->loop_s2) { // loop back
            qoa_mus_seek(q, q->loop_s1);
        } else if (q->spos == QOA_SLICE_LEN) { // decode a new slice
            q->cur_slice++;
            q->spos = 0;

            if ((q->cur_slice & QOA_FRAME_SLICES_MASK) == 0) {
                qoa_mus_next_frame(q);
            }
            qoa_mus_next_slice(q);
        }
    }
}

void qoa_mus_seek_ms(qoa_mus_s *q, u32 ms)
{
    qoa_mus_seek(q, (ms * 44100) / 1000);
}

void qoa_mus_seek(qoa_mus_s *q, u32 pos)
{
    i32 pfr    = pos / QOA_FRAME_SAMPLES;
    i32 stereo = q->seek >> 31;

    qoa_mus_rewind(q);
    q->cur_slice = pfr * QOA_FRAME_SLICES;

    // seek to frame header
    pltf_file_seek_cur(q->f, (((sizeof(u64) * QOA_FRAME_SLICES) << stereo) + sizeof(qoa_frameheader_s)) * pfr);
    qoa_mus_next_frame(q);
    qoa_mus_next_slice(q);

    q->pos = pos;
    u32 l  = pos - pfr * QOA_FRAME_SAMPLES;

    while (QOA_SLICE_LEN <= l) {
        l -= QOA_SLICE_LEN;

        for (i32 c = 0; c <= stereo; c++) {
            for (i32 i = 0; i < QOA_SLICE_LEN; i++) {
                qoa_decode_sample(&q->ds[c]);
            }
        }

        q->cur_slice++;
        qoa_mus_next_slice(q);
    }

    q->spos = l;
    for (i32 c = 0; c <= stereo; c++) {
        for (u32 i = 0; i < l; i++) {
            qoa_decode_sample(&q->ds[c]);
        }
    }
}

bool32 qoa_mus_active(qoa_mus_s *q)
{
    return (q->f != 0);
}

void qoa_sfx_rewind(qoa_sfx_s *q)
{
    q->cur_slice   = 0;
    q->spos        = 0;
    q->pos_pitched = 0;
    q->pos         = 0;
    qoa_decode_init(&q->ds);
    qoa_decode_init_slice(&q->ds, q->slices[0]);
}

bool32 qoa_sfx_start(qoa_sfx_s *q, u32 n_samples, void *dat, i32 p_q8, i32 v_q8, b32 repeat)
{
    if (!q || !n_samples || !dat || p_q8 <= 0) return 0;
    assert(((uptr)dat & 7) == 0); // 8 byte alignment
    q->slices      = (u64 *)dat;
    q->num_slices  = qoa_num_slices(n_samples);
    q->len         = n_samples;
    q->len_pitched = (n_samples * p_q8) >> 8;
    q->ipitch_q8   = 65536 / p_q8;
    q->v_q8        = v_q8;
    q->repeat      = repeat;
    qoa_sfx_rewind(q);
    assert((((q->len_pitched - 1) * q->ipitch_q8) >> 8) < n_samples);
    return 1;
}

void qoa_sfx_end(qoa_sfx_s *q)
{
    q->slices = 0;
}

b32 qoa_sfx_play(qoa_sfx_s *q, i16 *lbuf, i16 *rbuf, i32 len, i32 v_q8)
{
    i32 n_channels = 1 + (rbuf != 0);
    i32 pan_q8_l   = (0 < q->pan_q8 ? 256 - q->pan_q8 : 256);
    i32 pan_q8_r   = (0 > q->pan_q8 ? 256 + q->pan_q8 : 256);

    if (n_channels == 1) {
        // average the volume of l/r for mono
        pan_q8_l = (pan_q8_l + pan_q8_r) >> 1;
    }

    i32  v_q16[2] = {((i32)q->v_q8 * pan_q8_l * v_q8) >> 8,
                     ((i32)q->v_q8 * pan_q8_r * v_q8) >> 8};
    i16 *b[2]     = {lbuf, rbuf};

#if 0
    i32             len_decoded = len;
    bool32          sfx_ended   = 0;
    ALIGNAS(32) i16 samples[512];

    // decode into temporary buffer first, then write into audio buffer
    for (i32 n = 0; n < len; n++) {
        u32 p = (q->pos_pitched++ * q->ipitch_q8) >> 8;
        assert(p <= q->len);

        while (q->pos < p) { // seek and decode forward
            u32 n_samples = min_u32(QOA_SLICE_LEN - q->spos, p - q->pos);
            q->pos += n_samples;
            q->spos += n_samples;

            for (u32 i = 0; i < n_samples; i++) {
                q->sample = qoa_decode_sample(&q->ds);
            }

            if (q->spos == QOA_SLICE_LEN) { // decode a new slice
                q->spos = 0;
                q->cur_slice++;
                qoa_decode_init_slice(&q->ds, q->slices[q->cur_slice]);
            }
        }

        samples[n] = q->sample;
        if (q->pos_pitched == q->len_pitched) {
            if (q->repeat) {
                qoa_sfx_rewind(q);
            } else {
                qoa_sfx_end(q);
                sfx_ended   = 1;
                len_decoded = n + 1;
                break;
            }
        }
    }

    for (i32 n = 0; n < len_decoded; n++) {
        for (i32 c = 0; c < n_channels; c++) {
            *b[c] = i16_adds((i32)*b[c], mul_q16(v_q16[c], samples[n]));
            (b[c])++;
        }
    }

    return (sfx_ended ? 0 : 1);
#else
    for (i32 n = 0; n < len; n++) {
        u32 p = (q->pos_pitched++ * q->ipitch_q8) >> 8;
        assert(p <= q->len);

        while (q->pos < p) { // seek and decode forward
            u32 n_samples = min_u32(QOA_SLICE_LEN - q->spos, p - q->pos);
            q->pos += n_samples;
            q->spos += n_samples;

            for (u32 i = 0; i < n_samples; i++) {
                q->sample = qoa_decode_sample(&q->ds);
            }

            if (q->spos == QOA_SLICE_LEN) { // decode a new slice
                q->spos = 0;
                q->cur_slice++;
                qoa_decode_init_slice(&q->ds, q->slices[q->cur_slice]);
            }
        }

        for (i32 c = 0; c < n_channels; c++) {
            *b[c] = i16_adds((i32)*b[c], mul_q16(v_q16[c], q->sample));
            (b[c])++;
        }

        if (q->pos_pitched == q->len_pitched) {
            if (q->repeat) {
                qoa_sfx_rewind(q);
            } else {
                qoa_sfx_end(q);
                return 0;
            }
        }
    }
    return 1;
#endif
}

bool32 qoa_sfx_active(qoa_sfx_s *q)
{
    return (q->slices != 0);
}