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

static void qoa_decode_init(qoa_dec_s *d)
{
    d->h[0] = 0;
    d->h[1] = 0;
    d->w[0] = -(1 << 13);
    d->w[1] = +(2 << 13);
}

static void qoa_decode_init_slice(qoa_dec_s *d, u64 s)
{
    d->deq = (i16 *)qoa_deq[s >> 60];
    d->s   = s << 4;
}

static inline i32 qoa_decode_sample(qoa_dec_s *d)
{
    i32 dq = d->deq[d->s >> 61];
    d->s <<= 3;
    i32 pr = i16x2_dot(i16x2_ld(&d->w[0]), i16x2_ld(&d->h[0])) >> 13;
    i32 sp = ssat(pr + dq, 16);
    i32 dt = dq >> 4;
    d->w[0] += d->h[0] < 0 ? -dt : +dt;
    d->w[1] += d->h[1] < 0 ? -dt : +dt;
    d->h[0] = d->h[1];
    d->h[1] = sp;
    return sp;
}

void qoa_mus_next_slice(qoa_mus_s *q)
{
    if (q->slices_polled == q->slices_read) {
        u32 left = q->num_slices - q->cur_slice;
        u32 read = min_u32(left, QOA_SLICE_BUFFER);
        pltf_file_r(q->f, q->slices, sizeof(u64) * read);
        q->slices_read   = read;
        q->slices_polled = 0;
    }
    for (i32 n = 0; n < q->n_channels; n++) {
        qoa_decode_init_slice(&q->ds[n], q->slices[q->slices_polled++]);
    }
}

void qoa_mus_rewind(qoa_mus_s *q)
{
    pltf_file_seek_set(q->f, q->seek + sizeof(qoa_file_header_s));
    q->cur_slice     = 0;
    q->spos          = 0;
    q->slices_polled = 0;
    q->slices_read   = 0;
    q->pos           = 0;
    qoa_decode_init(&q->ds[0]);
    qoa_decode_init(&q->ds[1]);
}

bool32 qoa_mus_start(qoa_mus_s *q, void *f)
{
    if (!f) return 0;

    q->f                   = f;
    q->seek                = pltf_file_tell(f);
    q->v_q8                = 256;
    q->flags               = 1;
    qoa_file_header_s head = {0};
    pltf_file_seek_set(f, q->seek);
    pltf_file_r(f, &head, sizeof(qoa_file_header_s));
    q->num_slices = qoa_num_slices(head.num_samples) * 2;
    q->n_channels = head.num_channels;
    qoa_mus_rewind(q);
    qoa_mus_next_slice(q);
    return 1;
}

void qoa_mus_end(qoa_mus_s *q)
{
    if (q->f) {
        pltf_file_close(q->f);
        q->f = 0;
    }
    q->seek = 0;
}

static void qoa_mus_mo_st(qoa_dec_s *d, i32 n, i32 v, i16 *l, i16 *r)
{
    for (i32 k = 0; k < n; k++) {
        i32 s = qoa_decode_sample(d);
        i32 z = mul_q16(v, s);
        l[k]  = i16_adds((i32)l[k], z);
        r[k]  = i16_adds((i32)r[k], z);
    }
}

static void qoa_mus_mo_mo(qoa_dec_s *d, i32 n, i32 v, i16 *b)
{
    for (i32 k = 0; k < n; k++) {
        i32 s = qoa_decode_sample(d);
        i32 z = mul_q16(v, s);
        b[k]  = i16_adds((i32)b[k], z);
    }
}

static void qoa_mus_st_mo(qoa_dec_s *dl, qoa_dec_s *dr, i32 n, i32 v, i16 *b)
{
    for (i32 k = 0; k < n; k++) {
        i32 s = (qoa_decode_sample(dl) + qoa_decode_sample(dr)) >> 1;
        i32 z = mul_q16(v, s);
        b[k]  = i16_adds((i32)b[k], z);
    }
}

void qoa_mus(qoa_mus_s *q, i16 *lbuf, i16 *rbuf, i32 len)
{
    if (!q->seek) return;

    i16 *br    = rbuf;
    i16 *bl    = lbuf;
    u32  l     = (u32)len;
    i32  v_q16 = (i32)q->v_q8 << 8;
    i32  mode  = 0;
    switch (q->n_channels) {
    case 1: mode = (rbuf ? QOA_MUS_MODE_MO_ST : QOA_MUS_MODE_MO_MO); break;
    case 2: mode = (rbuf ? QOA_MUS_MODE_ST_ST : QOA_MUS_MODE_ST_MO); break;
    }

    q->pos += len;
    while (l) {
        u32 n = min_u32(QOA_SLICE_LEN - q->spos, l);
        l -= n;
        q->spos += n;

        switch (mode) {
        case QOA_MUS_MODE_ST_MO:
            qoa_mus_st_mo(&q->ds[0], &q->ds[1], n, v_q16, bl);
            bl += n;
            break;
        case QOA_MUS_MODE_MO_ST:
            qoa_mus_mo_st(&q->ds[0], n, v_q16, bl, br);
            bl += n;
            br += n;
            break;
        case QOA_MUS_MODE_MO_MO:
            qoa_mus_mo_mo(&q->ds[0], n, v_q16, bl);
            bl += n;
            break;
        case QOA_MUS_MODE_ST_ST:
            qoa_mus_mo_mo(&q->ds[0], n, v_q16, bl);
            qoa_mus_mo_mo(&q->ds[1], n, v_q16, br);
            bl += n;
            br += n;
            break;
        }

        if (q->spos == QOA_SLICE_LEN) { // decode a new slice
            q->spos = 0;
            q->cur_slice += q->n_channels;

            if (q->cur_slice == q->num_slices) { // new frame
                if (0) {
                    qoa_mus_end(q);
                    break;
                }
                qoa_mus_rewind(q);
            }
            qoa_mus_next_slice(q);
        }
    }
}

bool32 qoa_mus_active(qoa_mus_s *q)
{
    return (q->seek != 0);
}

void qoa_mus_set_vol(qoa_mus_s *q, i32 v)
{
    q->v_q8 = v;
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
    q->vol_q8      = v_q8;
    q->cur_slice   = 0;
    q->spos        = 0;
    q->pos_pitched = 0;
    q->pos         = 0;
    qoa_decode_init(&q->ds);
    qoa_decode_init_slice(&q->ds, q->slices[0]);
    assert((((q->len_pitched - 1) * q->ipitch_q8) >> 8) < n_samples);
    return 1;
}

void qoa_sfx_end(qoa_sfx_s *q)
{
    q->slices = 0;
}

void qoa_sfx_play(qoa_sfx_s *q, i16 *lbuf, i16 *rbuf, i32 len)
{
    i32 n_channels = 1 + (rbuf != 0);
    i32 pan_q8_l   = (0 < q->pan_q8 ? 256 - q->pan_q8 : 256);
    i32 pan_q8_r   = (0 > q->pan_q8 ? 256 + q->pan_q8 : 256);

    if (n_channels == 1) {
        pan_q8_l = (pan_q8_l + pan_q8_r) >> 1; // average the volume of l/r for mono
    }

    i32  v_q16[2] = {(i32)q->vol_q8 * pan_q8_l, (i32)q->vol_q8 * pan_q8_r};
    i16 *b[2]     = {lbuf, rbuf};

    for (i32 n = 0; n < len; n++) {
        u32 p = (q->pos_pitched++ * q->ipitch_q8) >> 8;
        assert(p <= q->len);

        while (q->pos < p) { // seek and decode forward
            u32 n_samples = min_u32(QOA_SLICE_LEN - q->spos, p - q->pos);
            q->pos += n_samples;
            q->spos += n_samples;

            for (u32 n = 0; n < n_samples; n++) {
                q->sample = qoa_decode_sample(&q->ds);
            }

            if (q->spos == QOA_SLICE_LEN) { // decode a new slice
                q->spos = 0;
                q->cur_slice++;
                qoa_decode_init_slice(&q->ds, q->slices[q->cur_slice]);
            }
        }

        for (u32 c = 0; c < n_channels; c++) {
            *b[c] = i16_adds((i32)*b[c], mul_q16(v_q16[c], q->sample));
            (b[c])++;
        }

        if (q->pos_pitched == q->len_pitched) {
            qoa_sfx_end(q);
            break;
        }
    }
}

bool32 qoa_sfx_active(qoa_sfx_s *q)
{
    return (q->slices != 0);
}