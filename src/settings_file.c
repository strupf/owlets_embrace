// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "settings_file.h"
#include "app.h"
#include "core/spm.h"
#include "pltf/pltf.h"
#include "util/mathfunc.h"

#define SETTINGS_FILENAME   "settings.bin"
#define SETTINGS_FILE_MAGIC 0x6553454F // "OESe"

typedef struct {
    u16 checksum;
    u8  version;  // settings file version
    u8  platform; // platform used to produce this file
    u32 magic;
    u8  unused[8];
} settings_file_header_s;

// reads content from file and verifies checksum
static err32 settings_file_r_buffer(void *f, void *buf, usize buf_size, u32 checksum)
{
    if (pltf_file_r_checked(f, buf, buf_size)) return SETTINGS_FILE_ERR_RW;
    return (checksum == crc16_aligned4(0xFFFF, buf, buf_size) ? 0 : SETTINGS_FILE_ERR_CHECKSUM);
}

err32 settings_r()
{
    void *f = pltf_file_open_r(SETTINGS_FILENAME);
    if (!f) return SETTINGS_FILE_ERR_OPEN;

    err32                  err = 0;
    settings_s            *s   = &SETTINGS;
    settings_file_s        sf  = {0};
    settings_file_header_s h   = {0};

    if (pltf_file_r_checked(f, &h, sizeof(settings_file_header_s)) && h.magic == SETTINGS_FILE_MAGIC) {
        switch (h.version) {
        case SETTINGS_FILE_VERSION:
            err |= settings_file_r_buffer(f, &sf, sizeof(settings_file_s), h.checksum);
            break;
        default:
            err |= SETTINGS_FILE_ERR_VERSION;
            break;
        }
    } else {
        err |= SETTINGS_FILE_ERR_RW;
    }

    if (!pltf_file_close(f)) {
        err |= SETTINGS_FILE_ERR_CLOSE;
    }
    if (err) {
        settings_default();
    } else {
        *s = sf.s;
    }
    return err;
}

err32 settings_w()
{
    void *f = pltf_file_open_w(SETTINGS_FILENAME);
    if (!f) return SETTINGS_FILE_ERR_OPEN;

    err32                  err = 0;
    settings_s            *s   = &SETTINGS;
    settings_file_s        sf  = {0};
    settings_file_header_s h   = {0};
    sf.s                       = *s; // populate settings file

    h.version  = SETTINGS_FILE_VERSION;
    h.platform = APP_VERSION_PLATFORM;
    h.magic    = SETTINGS_FILE_MAGIC;
    h.checksum = crc16_aligned4(0xFFFF, &sf, sizeof(settings_file_s));

    if (!pltf_file_w_checked(f, &h, sizeof(settings_file_header_s)) ||
        !pltf_file_w_checked(f, &sf, sizeof(settings_file_s))) {
        err |= SETTINGS_FILE_ERR_RW;
    }
    if (!pltf_file_close(f)) {
        err |= SETTINGS_FILE_ERR_CLOSE;
    }
    return err;
}