// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "settings.h"
#include "app.h"
#include "pltf/pltf.h"

typedef struct {
    u32 version;
    u16 checksum;
    u8  unused[2];
} settings_header_s;

#define SETTINGS_FILE_NAME "settings.bin"

settings_s SETTINGS;

void settings_load_default();

i32 settings_load()
{
#if PLTF_DEBUG && 0 // just load and override default settings for dev purposes
    settings_load_default();
    return 0;
#else
    settings_header_s h   = {0};
    i32               res = 0;

    void *f = pltf_file_open_r(SETTINGS_FILE_NAME);
    if (!f) {
        res |= SETTINGS_ERR_OPEN;
        goto RET;
    }

    if (!pltf_file_rs(f, &h, sizeof(settings_header_s))) {
        res |= SETTINGS_ERR_RW;
        goto CLOSEF;
    }

    switch (h.version) {
    case GAME_VERSION:
        if (pltf_file_rs(f, &SETTINGS, sizeof(settings_s))) {
            if (h.checksum != crc16(&SETTINGS, sizeof(settings_s))) {
                res |= SETTINGS_ERR_CHECKSUM;
            }
        } else {
            res |= SETTINGS_ERR_RW;
        }
        break;
    default: // reading unsupported file
        res |= SETTINGS_ERR_VERSION;
        break;
    }

CLOSEF:;
    if (!pltf_file_close(f)) {
        res |= SETTINGS_ERR_CLOSE;
    }
RET:;
    if (res) {
        settings_load_default();
    }
    return res;
#endif
}

i32 settings_save()
{
    void *f = pltf_file_open_w(SETTINGS_FILE_NAME);
    if (!f) return SETTINGS_ERR_OPEN;

    i32               res = 0;
    settings_header_s h   = {GAME_VERSION,
                             crc16(&SETTINGS, sizeof(settings_s))};
    if (!pltf_file_ws(f, &h, sizeof(settings_header_s)) ||
        !pltf_file_ws(f, &SETTINGS, sizeof(settings_s))) {
        res |= SETTINGS_ERR_RW;
    }

    if (pltf_file_close(f)) {
        res |= SETTINGS_ERR_CLOSE;
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
    s->hook_mode         = HERO_HOOK_B_TIMED;
}