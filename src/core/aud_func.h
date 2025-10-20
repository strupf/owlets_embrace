// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#if 0
#undef AUD_FUNC_STEREO
#define AUD_FUNC_STEREO 1
#error "aud func editing enabled"
#endif

#include "core/aud.h"
#include "core/qoa.h"
#include "util/mathfunc.h"

// called from audio context
AUDIO_CTX void mus_channel_track_reset(mus_channel_track_s *tr);
AUDIO_CTX void mus_start_queued(mus_channel_s *ch);
AUDIO_CTX void mus_on_tick(mus_channel_s *ch, i32 len);

ATTRIBUTE_SECTION(".text.audio")
#if AUD_FUNC_STEREO
AUDIO_CTX void mus_channel_stereo(aud_s *a, mus_channel_s *ch, i16 *lbuf, i16 *rbuf, i32 len)
#else
AUDIO_CTX void mus_channel_mono(aud_s *a, mus_channel_s *ch, i16 *lbuf, i32 len)
#endif
{
    if (!ch->musID) {
        if (!ch->musID_queued) {
            return;
        }
        mus_start_queued(ch);
    }
    mus_on_tick(ch, len);

    aud_vol_handler_s *vol = &ch->vol;
#if AUD_FUNC_STEREO
    i16 *rb = rbuf;
#endif
    i16 *lb = lbuf;
    i32  ll = len;

    do {
        i32 l_sub = aud_vol_handler_update(vol, ll);
        i32 v_q16 = aud_vol_handler_v_q8_loudness(vol) * (i32)a->v_q8_mus;

        for (i32 n = 0; n < NUM_MUS_CHANNEL_TRACKS; n++) {
            mus_channel_track_s *tr = &ch->tracks[n];
            qoa_stream_s        *q  = &tr->q;
            if (!q->f) continue;

            // apply minimal smooth to track's volume if needed to avoid popping
            // mus_channel is in control of more gradually fading of tracks if needed
            // fades from 0 to 256 (max) in 256 / 44100Hz = 5.8 ms
            tr->v_q8 += sgn_i32((i32)tr->v_q8_dst - (i32)tr->v_q8);
            i32 v_q16_tr = ((i32)tr->v_q8 * v_q16) >> 8;
#if AUD_FUNC_STEREO
            qoa_stream_stereo(q, lb, rb, l_sub, v_q16_tr, v_q16_tr);
#else
            qoa_stream_mono(q, lb, l_sub, v_q16_tr);
#endif

            if (!q->f) {
                mus_channel_track_reset(tr);
            }
        }

        ll -= l_sub;
        lb += l_sub;
#if AUD_FUNC_STEREO
        rb += l_sub;
#endif
    } while (ll);

    if (ch->musID_queued && aud_vol_handler_v_q8(vol) == 0) {
        ch->musID = 0;
        for (i32 n = 0; n < NUM_MUS_CHANNEL_TRACKS; n++) {
            mus_channel_track_reset(&ch->tracks[n]);
        }
    }
}

ATTRIBUTE_SECTION(".text.audio")
#if AUD_FUNC_STEREO
AUDIO_CTX void sfx_channel_stereo(aud_s *a, sfx_channel_s *ch, i16 *lbuf, i16 *rbuf, i32 len)
#else
AUDIO_CTX void sfx_channel_mono(aud_s *a, sfx_channel_s *ch, i16 *lbuf, i32 len)
#endif
{
    qoa_data_s *q = &ch->q;
    if (!qoa_data_active(q)) return;

    i32 l_q8 = 0;
    i32 r_q8 = 0;
    i32 m_q8 = aud_calc_positional_vol(a, a->v_q8_sfx, ch->px, ch->py, ch->r, 9, &l_q8, &r_q8);

    ch->v_q8 += sgn_i32((i32)ch->v_q8_dst - (i32)ch->v_q8);
    i32 v_q16 = (i32)ch->v_q8 * (i32)ch->v_q8;

#if AUD_FUNC_STEREO
    qoa_data_stereo(q, lbuf, rbuf, len, (l_q8 * v_q16) >> 8, (r_q8 * v_q16) >> 8);
#else
    qoa_data_mono(q, lbuf, len, (m_q8 * v_q16) >> 8);
#endif

    if (!qoa_data_active(q) || ch->v_q8 == 0) {
        mclr(ch, sizeof(sfx_channel_s));
    }
}