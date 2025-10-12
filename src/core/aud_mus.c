// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "core/aud.h"

AUDIO_CTX void mus_start_queued(mus_channel_s *ch);

AUDIO_CTX static void mus_channel_track_play(mus_channel_s *ch, i32 track_index, const void *str, i32 v_q8, i32 pos_beg, i32 loop_pos_beg, i32 loop_pos_end, b32 repeat)
{
    DEBUG_LOG("QOA STREAM start...\n");
    void                *f  = wad_open_str(str, 0, 0);
    mus_channel_track_s *tr = &ch->tracks[track_index];
    qoa_stream_s        *q  = &tr->q;
    tr->v_q8                = v_q8;
    tr->v_q8_dst            = v_q8;
    qoa_stream_start(q, f, pos_beg, loop_pos_beg, loop_pos_end, repeat);
    DEBUG_LOG("QOA STREAM started\n");
}

AUDIO_CTX void mus_channel_track_reset(mus_channel_track_s *tr)
{
    qoa_stream_end(&tr->q);
    tr->v_q8     = 0;
    tr->v_q8_dst = 0;
}

AUDIO_CTX void mus_start_queued(mus_channel_s *ch)
{
    DEBUG_LOG("mus start queued %p...\n", ch);
    aud_vol_handler_init_dt(&ch->vol, 256, 0);
    ch->musID        = ch->musID_queued;
    ch->musID_queued = 0;

    switch (ch->musID) {
    case MUSID_CAVE: {
        mus_channel_track_play(ch, 0, "M_CAVE", 256, 0, 498083, 0, 1);
        break;
    }
    case MUSID_WATERFALL: {
        mus_channel_track_play(ch, 0, "M_WATERFALL", 256, 0, 226822, 0, 1);
        break;
    }
    case MUSID_ANCIENT_TREE: {
        mus_channel_track_play(ch, 0, "M_ANCIENT_TREE", 256, 0, 564480, 0, 1);
        break;
    }
    case MUSID_SNOW: {
        mus_channel_track_play(ch, 0, "M_SNOW", 256, 0, 368146, 0, 1);
        break;
    }
    case MUSID_FOREST: {
        mus_channel_track_play(ch, 0, "M_FOREST", 256, 0, 769768, 0, 1);
        break;
    }
    case MUSID_INTRO: {
        mus_channel_track_play(ch, 0, "M_INTRO", 256, 0, 325679, 0, 1);
        break;
    }
    }
}

AUDIO_CTX void mus_on_cue(mus_channel_s *ch, i32 musID, i32 v_q8, i32 millis_fade_out_cur)
{
    ch->v_q8_trg_game = v_q8; // intended target volume of the new track to be queued, independent from current fading state

    if (ch->musID == musID) {
        // bring volume back up (if necessary)
        aud_vol_handler_init_millis(&ch->vol, v_q8, 1000);
    } else {
        ch->musID_queued = musID;
        if (ch->musID) {
            aud_vol_handler_init_millis(&ch->vol, 0, millis_fade_out_cur);
        }
    }
}

// programming of specific audio logic
AUDIO_CTX void mus_on_tick(mus_channel_s *ch, i32 len)
{
    // programming of individual pieces?
    switch (ch->musID) {
    default: break;
    }
}

void mus_cue(i32 channel_index, i32 musID, i32 millis_fade_out_cur)
{
    aud_cmd_s      cmd = aud_cmd_gen(AUD_CMD_MUS_CUE);
    aud_cmd_mus_s *cc  = &cmd.mus;
    cc->channel_index  = channel_index;
    cc->musID          = musID;
    cc->millis_fade    = millis_fade_out_cur;
    aud_cmd_push(cmd);
}

void mus_volume(i32 channel_index, i32 v_q8, i32 millis_fade)
{
    aud_cmd_s      cmd = aud_cmd_gen(AUD_CMD_MUS_ADJUST_VOL);
    aud_cmd_mus_s *cc  = &cmd.mus;
    cc->channel_index  = channel_index;
    cc->v_q8           = v_q8;
    cc->millis_fade    = millis_fade;
    aud_cmd_push(cmd);
}

AUDIO_CTX void mus_channel_reset(mus_channel_s *ch)
{
    for (i32 n = 0; n < NUM_MUS_CHANNEL_TRACKS; n++) {
        mus_channel_track_reset(&ch->tracks[n]);
    }
    ch->musID         = 0;
    ch->musID_queued  = 0;
    ch->v_q8_trg_game = 0;
    ch->seed          = 0;
    mclr(&ch->vol, sizeof(ch->vol));
    mclr(ch->v, sizeof(ch->v));
}
