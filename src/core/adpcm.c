// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "adpcm.h"
#include "util/mathfunc.h"

static i32 adpcm_advance_step(adpcm_s *pcm);

void adpcm_reset_to_start(adpcm_s *pcm)
{
    pcm->step_size   = 127;
    pcm->hist        = 0;
    pcm->nibble      = 0;
    pcm->pos         = 0;
    pcm->pos_pitched = 0;
    pcm->data_pos    = 0;
    pcm->curr_byte   = 0;
}

void adpcm_set_pitch(adpcm_s *pcm, i32 pitch_q8)
{
    if (pitch_q8 <= 1) return;
    pcm->pitch_q8    = pitch_q8;
    pcm->ipitch_q8   = (256 << 8) / pcm->pitch_q8;
    pcm->len_pitched = (pcm->len * pcm->pitch_q8) >> 8;

    // highest position in pitched len shall not be out of bounds
    assert((((pcm->len_pitched - 1) * pcm->ipitch_q8) >> 8) < pcm->len);
}

void adpcm_playback_nonpitch_silent(adpcm_s *pcm, i32 len)
{
    pcm->pos_pitched += len;
    adpcm_advance_to(pcm, pcm->pos_pitched);
}

void adpcm_playback(adpcm_s *pcm, i16 *lb, i16 *rb, i32 len)
{
    i16 *l = lb;
    for (i32 i = 0; i < len; i++, l++) {
        u32 p = (++pcm->pos_pitched * pcm->ipitch_q8) >> 8;
        i32 v = (adpcm_advance_to(pcm, p) * pcm->vol_q8) >> 8;
        *l    = ssat16((i32)*l + v);
    }
}

void adpcm_playback_nonpitch(adpcm_s *pcm, i16 *lb, i16 *rb, i32 len)
{
    i16 *l = lb;
    pcm->pos_pitched += len;
    for (i32 i = 0; i < len; i++, l++) {
        i32 v = (adpcm_advance_step(pcm) * pcm->vol_q8) >> 8;
        *l    = ssat16((i32)*l + v);
    }
}

i32 adpcm_advance_to(adpcm_s *pcm, u32 pos)
{
    assert(pos < pcm->len);
    assert(pcm->pos <= pos);
    while (pcm->pos < pos) { // can't savely skip any samples with ADPCM
        adpcm_advance_step(pcm);
    }
    return pcm->hist;
}

// an ADPCM function to advance the sample decoding
// github.com/superctr/adpcm/blob/master/ymb_codec.c
static i32 adpcm_advance_step(adpcm_s *pcm)
{
    static const u8 t_step[8] = {57, 57, 57, 57, 77, 102, 128, 153};

    assert(pcm->pos + 1 < pcm->len);
    pcm->pos++;
    if (!pcm->nibble) {
        pcm->curr_byte = pcm->data[pcm->data_pos++];
    }
    u32 b = (pcm->curr_byte << pcm->nibble) >> 4;
    pcm->nibble ^= 4;

    i32 s          = b & 7;
    i32 d          = ((1 + (s << 1)) * pcm->step_size) >> 3;
    pcm->step_size = clamp_i32((t_step[s] * pcm->step_size) >> 6, 127, 24576);
    pcm->hist      = ssat16((i32)pcm->hist + ((b & 8) ? -d : +d));
    return pcm->hist;
}