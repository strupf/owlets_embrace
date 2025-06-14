// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "settings.h"
#include "app.h"
#include "pltf/pltf.h"

typedef struct {
    ALIGNAS(8)
    u32 version;
    u16 checksum;
    u8  unused[2];
} settings_header_s;

static_assert(sizeof(settings_header_s) == 8, "size settings header");

#define SETTINGS_FILE_NAME "settings.bin"

settings_s SETTINGS;

err32 settings_load(settings_s *s)
{
#if 1 // just load and override default settings for dev purposes
    settings_default(s);
    return 0;
#endif
    void *f = pltf_file_open_r(SETTINGS_FILE_NAME);
    if (!f) return SETTINGS_ERR_OPEN;

    settings_header_s h   = {0};
    err32             res = 0;

    if (pltf_file_r_checked(f, &h, sizeof(settings_header_s))) {
        game_version_s v = game_version_decode(h.version);

        if (v.vmaj == 0) {
            res |= SETTINGS_ERR_VERSION;
        } else {
            if (pltf_file_r_checked(f, s, sizeof(settings_s))) {
                if (h.checksum != crc16(s, sizeof(settings_s))) {
                    res |= SETTINGS_ERR_CHECKSUM;
                }
            } else {
                res |= SETTINGS_ERR_RW;
            }
        }
    } else {
        res |= SETTINGS_ERR_RW;
        settings_default(s);
    }

    if (!pltf_file_close(f)) {
        res |= SETTINGS_ERR_CLOSE;
    }
    return res;
}

err32 settings_save(settings_s *s)
{
    void *f = pltf_file_open_w(SETTINGS_FILE_NAME);
    if (!f) return SETTINGS_ERR_OPEN;

    err32             res = 0;
    settings_header_s h   = {0};
    h.version             = GAME_VERSION;
    h.checksum            = crc16(s, sizeof(settings_s));

    if (!pltf_file_w_checked(f, &h, sizeof(settings_header_s)) ||
        !pltf_file_w_checked(f, s, sizeof(settings_s))) {
        res |= SETTINGS_ERR_RW;
    }

    if (pltf_file_close(f)) {
        res |= SETTINGS_ERR_CLOSE;
    }
    return res;
}

void settings_default(settings_s *s)
{
    mclr(s, sizeof(settings_s));
    s->mode              = SETTINGS_MODE_NORMAL;
    s->shake_smooth      = SETTINGS_SHAKE_SMOOTH_MAX / 2;
    s->shake_sensitivity = SETTINGS_SHAKE_SENS_MAX / 2;
    s->vol_mus           = SETTINGS_VOL_MAX;
    s->vol_sfx           = SETTINGS_VOL_MAX;
    s->swap_ticks        = (SETTINGS_SWAP_TICKS_MIN + SETTINGS_SWAP_TICKS_MAX) / 2;
}