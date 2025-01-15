// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "settings.h"
#include "app.h"
#include "pltf/pltf.h"

typedef struct {
    u32 version;
    u8  unused[12];
} settings_header_s;

static_assert(sizeof(settings_header_s) == 16, "settings header size");

#define SETTINGS_FILE_NAME "settings.bin"

settings_s SETTINGS;

void settings_load_default();

i32 settings_load()
{
#if PLTF_DEBUG // just load and override default settings for dev purposes
    settings_load_default();
    return 0;
#else
    void *f = pltf_file_open_r(SETTINGS_FILE_NAME);
    if (!f) {
        settings_load_default();
        return SETTINGS_ERR_OPEN;
    }

    i32               res    = 0;
    i32               br     = 0;
    usize             br_ref = sizeof(settings_header_s);
    settings_header_s h      = {0};
    br += pltf_file_r(f, &h, sizeof(settings_header_s));

    switch (h.version) {
    case GAME_VERSION:
        br_ref += sizeof(settings_s);
        br += pltf_file_r(f, &SETTINGS, sizeof(settings_s));
        break;
    default: // reading unsupported file
        settings_load_default();
        res |= SETTINGS_ERR_VERSION;
        break;
    }

    if (!pltf_file_close(f)) {
        res |= SETTINGS_ERR_CLOSE;
    }
    if (br != (i32)br_ref) {
        res |= SETTINGS_ERR_RW;
    }
    return res;
#endif
}

i32 settings_save()
{
    void *f = pltf_file_open_w(SETTINGS_FILE_NAME);
    if (!f) return SETTINGS_ERR_OPEN;

    i32               res = 0;
    i32               bw  = 0;
    settings_header_s h   = {GAME_VERSION};
    bw += pltf_file_w(f, &h, sizeof(settings_header_s));
    bw += pltf_file_w(f, &SETTINGS, sizeof(settings_s));

    if (pltf_file_close(f)) {
        res |= SETTINGS_ERR_CLOSE;
    }
    if (bw != (i32)(sizeof(settings_s) + sizeof(settings_s))) {
        res |= SETTINGS_ERR_RW;
    }
    return res;
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
}