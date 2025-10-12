// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "core/aud.h"

AUDIO_CTX sfx_channel_s *sfx_channel_find(aud_s *a, i32 UID)
{
    for (i32 n = 0; n < NUM_SFX_CHANNELS; n++) {
        sfx_channel_s *ch = &a->sfx_channels[n];
        if (ch->UID == UID) return ch;
    }
    return 0;
}

i32 sfx_cue(i32 sfxID, i32 v_q8, i32 pitch_q8)
{
    return sfx_cue_ext(sfxID, v_q8, pitch_q8, 0);
}

i32 sfx_cuef(i32 sfxID, f32 v, f32 pitch)
{
    return sfx_cue(sfxID, Q_8(v), Q_8(pitch));
}

i32 sfx_cue_ext(i32 sfxID, i32 v_q8, i32 pitch_q8, bool32 repeat)
{
    aud_s *a = &g_AUD;
    if (a->sfx_blocked) return 0;

    sfx_s          sfx = asset_sfx(sfxID);
    aud_cmd_s      cmd = aud_cmd_gen(AUD_CMD_SFX_PLAY);
    aud_cmd_sfx_s *cc  = &cmd.sfx;
    a->sfx_UID         = a->sfx_UID == I32_MAX ? 1 : a->sfx_UID + 1;
    i32 UID            = a->sfx_UID;

    cc->UID      = UID;
    cc->data     = sfx.data;
    cc->v_q8     = v_q8;
    cc->pitch_q8 = pitch_q8;
    if (repeat) {
        cmd.flags |= AUD_CMD_FLAG_REPEAT;
    }
    aud_cmd_push(cmd);
    return UID;
}

i32 sfx_cuef_ext(i32 sfxID, f32 v, f32 pitch, bool32 repeat)
{
    return sfx_cue_ext(sfxID, Q_8(v), Q_8(pitch), repeat);
}

i32 sfx_cue_pos(i32 sfxID, i32 v_q8, i32 pitch_q8, bool32 repeat, i32 px, i32 py, i32 r)
{
    i32 UID = sfx_cue_ext(sfxID, v_q8, pitch_q8, repeat);
    if (UID == 0) return 0;

    sfx_set_pos(UID, px, py, r);
    return UID;
}

i32 sfx_cuef_pos(i32 sfxID, f32 v, f32 pitch, bool32 repeat, i32 px, i32 py, i32 r)
{
    return sfx_cue_pos(sfxID, Q_8(v), Q_8(pitch), repeat, px, py, r);
}

void sfx_set_pos(i32 UID, i32 px, i32 py, i32 r)
{
    aud_cmd_s          cmd = aud_cmd_gen(AUD_CMD_SFX_POS);
    aud_cmd_sfx_pos_s *cc  = &cmd.sfx_pos;
    cmd.v16                = min_i32(r, U16_MAX);
    cc->UID                = UID;
    cc->px                 = px;
    cc->py                 = py;
    aud_cmd_push(cmd);
}

void sfx_stop(u32 UID, u32 millis_fade)
{
    aud_cmd_s      cmd = aud_cmd_gen(AUD_CMD_SFX_STOP);
    aud_cmd_sfx_s *cc  = &cmd.sfx;
    cc->UID            = UID;
    aud_cmd_push(cmd);
}

void sfx_stop_all()
{
    aud_cmd_s cmd = aud_cmd_gen(AUD_CMD_SFX_STOP_ALL);
    aud_cmd_push(cmd);
}

void sfx_block_new(bool32 blocked)
{
    aud_s *a       = &g_AUD;
    a->sfx_blocked = blocked;
}
