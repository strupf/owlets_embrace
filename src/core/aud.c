// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/aud.h"
#include "gamedef.h"
#include "sys/sys.h"

AUD_s AUD;

void aud_update()
{
    switch (AUD.mus_fade) {
    case MUS_FADE_NONE: return;
    case MUS_FADE_OUT: {
        AUD.mus_fade_ticks--;
        sys_set_mus_vol((AUD.mus_fade_ticks << 8) / AUD.mus_fade_ticks_max);
        if (AUD.mus_fade_ticks > 0) break;

        if (sys_mus_play(AUD.mus_new) != 0) {
            AUD.mus_fade = MUS_FADE_NONE;
            return;
        }
        AUD.mus_fade           = MUS_FADE_IN;
        AUD.mus_fade_ticks     = AUD.mus_fade_in;
        AUD.mus_fade_ticks_max = AUD.mus_fade_ticks;
    } break;
    case MUS_FADE_IN: {
        AUD.mus_fade_ticks--;
        sys_set_mus_vol(256 - ((AUD.mus_fade_ticks << 8) / AUD.mus_fade_ticks_max));
        if (AUD.mus_fade_ticks > 0) break;

        sys_set_mus_vol(256);
        AUD.mus_fade = MUS_FADE_NONE;
    } break;
    }
}

snd_s snd_load(const char *pathname, void *(*allocf)(usize s))
{
    snd_s s = {0};
    s.wav   = sys_load_wavdata(pathname, allocf);
    return s;
}

mus_s mus_load(const char *pathname)
{
    mus_s m = {0};
    str_cpy(m.path, pathname);
    return m;
}

void snd_play(snd_s s)
{
    snd_play_ext(s, 1.f, 1.f);
}

void snd_play_ext(snd_s s, float vol, float pitch)
{
    sys_wavdata_play(s.wav, vol, pitch);
}

void mus_fade_to(mus_s m, int ticks_out, int ticks_in)
{
    AUD.mus_fade_in = ticks_in;
    AUD.mus_fade    = MUS_FADE_OUT;
    if (sys_mus_playing()) {
        AUD.mus_fade_ticks_max = ticks_out;
    } else {
        AUD.mus_fade_ticks_max = 1;
    }
    AUD.mus_fade_ticks = AUD.mus_fade_ticks_max;
    str_cpy(AUD.mus_new, m.path);
}

void mus_stop()
{
    sys_mus_stop();
}

bool32 mus_playing()
{
    return sys_mus_playing();
}