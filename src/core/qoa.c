// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// https://qoaformat.org/

#include "core/qoa.h"
#include "pltf/pltf.h"
#include "pltf/pltf_intrin.h"
#include "util/mathfunc.h"

#define QOA_FUNC_STEREO 0
#include "core/qoa_func.h"
#undef QOA_FUNC_STEREO

#define QOA_FUNC_STEREO 1
#include "core/qoa_func.h"
#undef QOA_FUNC_STEREO

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

typedef struct {
    ALIGNAS(16)
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
    i32 dt = dq >> 4;
    i32 pr = i16x2_dot(i16x2_ld(&d->lms.w[0]), i16x2_ld(&d->lms.h[0])) >> 13;
    i32 sp = pr + dq;
#if 0 // needs clamp? -> see output of audio converter if any source clips
    sp = ssat(sp, 16);
#endif
    d->lms.w[0] += d->lms.h[0] < 0 ? -dt : +dt;
    d->lms.w[1] += d->lms.h[1] < 0 ? -dt : +dt;
    d->lms.h[0] = d->lms.h[1];
    d->lms.h[1] = sp;
    return sp;
}

// STREAM
static void qoa_stream_next_slice(qoa_stream_s *q);
static void qoa_stream_rewind(qoa_stream_s *q);

static void qoa_stream_next_slice(qoa_stream_s *q)
{
    // initalize a new frame if needed
    u8 cur_slice_in_frame = (u8)(q->pos / QOA_SLICE_LEN);
    if (cur_slice_in_frame == 0) {
        qoa_frameheader_s h = {0};
        pltf_file_r(q->f, &h, sizeof(qoa_frameheader_s));
        q->ds[0].lms = h.lms[0];
    }

    // refill slice buffer from file if needed
    u32 slice_index_in_buf = cur_slice_in_frame & QOA_FRAME_SLICES_BUF_MASK;
    if (slice_index_in_buf == 0) {
        pltf_file_r(q->f, q->slices, sizeof(u64) * QOA_STREAM_SLICES_BUFFERED);
    }

    // initialize new slice
    u64 *p_slice = &q->slices[slice_index_in_buf];
    qoa_decode_init_slice(&q->ds[0], *p_slice);
    QOA_PREFETCH(p_slice + 1);
}

static void qoa_stream_rewind(qoa_stream_s *q)
{
    pltf_file_seek_set(q->f, q->seek);
    q->pos = 0;
    qoa_decode_init(&q->ds[0]);
}

bool32 qoa_stream_start(qoa_stream_s *q, void *f, i32 pos_beg, i32 loop_pos_beg, i32 loop_pos_end, b32 repeat)
{
    if (!f || !q) return 0;

    mclr(q, sizeof(qoa_stream_s));

    qoa_file_header_s h = {0};
    pltf_file_r(f, &h, sizeof(qoa_file_header_s));
    q->seek        = (u32)pltf_file_tell(f);
    q->repeat      = repeat;
    q->f           = f;
    q->num_samples = h.num_samples;
    assert(h.num_channels == 1);
    qoa_stream_set_loop(q, loop_pos_beg, loop_pos_end);
    qoa_stream_seek(q, pos_beg);
    return 1;
}

void qoa_stream_set_loop(qoa_stream_s *q, i32 loop_pos_beg, i32 loop_pos_end)
{
    q->loop_pos_beg = loop_pos_beg;
    q->loop_pos_end = loop_pos_end ? loop_pos_end : q->num_samples;
}

void qoa_stream_end(qoa_stream_s *q)
{
    if (q->f) {
        pltf_file_close(q->f);
        q->f = 0;
    }
}

void qoa_stream_seek(qoa_stream_s *q, i32 sample_pos)
{
    qoa_stream_rewind(q);

    i32 pfr = max_i32(0, sample_pos) / QOA_FRAME_SAMPLES;

    // seek to frame header
    pltf_file_seek_cur(q->f, (((sizeof(u64) * QOA_FRAME_SLICES)) + sizeof(qoa_frameheader_s)) * pfr);
    qoa_stream_next_slice(q);

    if (0 < sample_pos) {
        q->pos = pfr * QOA_FRAME_SAMPLES;
        u32 l  = sample_pos - pfr * QOA_FRAME_SAMPLES;

        while (QOA_SLICE_LEN <= l) {
            q->pos += QOA_SLICE_LEN;
            l -= QOA_SLICE_LEN;

            for (i32 i = 0; i < QOA_SLICE_LEN; i++) {
                qoa_decode_sample(&q->ds[0]);
            }
            qoa_stream_next_slice(q);
        }

        q->pos += l;
        for (u32 i = 0; i < l; i++) {
            qoa_decode_sample(&q->ds[0]);
        }
    } else {
        // delayed playback, but still have to initialize first slice
        q->pos = sample_pos;
    }
}

bool32 qoa_stream_active(qoa_stream_s *q)
{
    return (q->f != 0);
}

void qoa_data_rewind(qoa_data_s *q)
{
    q->pos_pitched = 0;
    q->pos         = 0;
    qoa_decode_init(&q->ds);
    qoa_decode_init_slice(&q->ds, q->slices[0]);
}

bool32 qoa_data_start(qoa_data_s *q, void *data, i32 pitch_q8, i32 pan_q8, b32 repeat)
{
    if (!q || !data || pitch_q8 <= 1) return 0;
    assert(((uptr)data & 7) == 0); // 8 byte alignment

    qoa_file_header_s *h = (qoa_file_header_s *)data;
    q->slices            = (u64 *)((byte *)data + sizeof(qoa_file_header_s));
    QOA_PREFETCH(q->slices);
    q->pitch_q8    = pitch_q8;
    q->len_pitched = (i32)(((u32)h->num_samples << 8) / (u32)pitch_q8);
    q->repeat      = repeat;
    qoa_data_rewind(q);
    assert((((q->len_pitched - 1) * (i32)q->pitch_q8) >> 8) < (i32)h->num_samples);
    return 1;
}

void qoa_data_end(qoa_data_s *q)
{
    q->slices = 0;
}

#if 0
void qoa_dat_seek(qoa_dat_s *q, i32 pos)
{
    qoa_dat_rewind(q);

    while (q->pos < pos) {                      // seek and decode forward
        i32 spos      = q->pos % QOA_SLICE_LEN; // position in slice
        u32 n_samples = min_u32(QOA_SLICE_LEN - spos, pos - q->pos);
        q->pos += n_samples;

        for (u32 i = 0; i < n_samples; i++) {
            q->sample = qoa_decode_sample(&q->ds);
        }

        if (spos + n_samples == QOA_SLICE_LEN) { // decode a new slice
            qoa_decode_init_slice(&q->ds, q->slices[q->pos / QOA_SLICE_LEN]);
        }
    }
    q->pos_pitched = min_i32((q->pos << 8) / q->pitch_q8, q->len_pitched - 1);
}
#endif

bool32 qoa_data_active(qoa_data_s *q)
{
    return (q->slices != 0);
}