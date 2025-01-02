// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "settings.h"
#include "app.h"
#include "pltf/pltf.h"

#define SETTINGS_FILE_NAME "settings.bin"

settings_s SETTINGS;

void settings_load_default();

void settings_load()
{
#if 1 // just load and override default settings for dev purposes
    settings_load_default();
#else
    void *f = pltf_file_open_r(SETTINGS_FILE_NAME);
    if (!f) {
        settings_load_default();
        return;
    }

    u32 version = 0;
    pltf_file_r(f, &version, sizeof(u32));

    switch (version) {
    case GAME_VERSION:
        pltf_file_r(f, &SETTINGS, sizeof(settings_s));
        break;
    default: // reading unsupported file
        settings_load_default();
        break;
    }

    pltf_file_close(f);
#endif
}

void settings_save()
{
    void *f = pltf_file_open_w(SETTINGS_FILE_NAME);
    if (!f) return;

    u32 version = GAME_VERSION;
    pltf_file_w(f, &version, sizeof(u32));
    pltf_file_w(f, &SETTINGS, sizeof(settings_s));
    pltf_file_close(f);
}

void settings_load_default()
{
    settings_s *s = &SETTINGS;
    mclr(s, sizeof(settings_s));
    s->shake_smooth      = SETTINGS_SHAKE_SMOOTH_MAX / 2;
    s->shake_sensitivity = SETTINGS_SHAKE_SENS_MAX / 2;
    s->vol_mus           = SETTINGS_VOL_MAX;
    s->vol_sfx           = SETTINGS_VOL_MAX;
    s->ticks_hook_hold   = SETTINGS_TICKS_HOOK_CONTROL;
    settings_save();
}