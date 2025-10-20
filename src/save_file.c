// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "save_file.h"
#include "core/spm.h"
#include "pltf/pltf.h"
#include "util/mathfunc.h"

#define SAVE_FILE_MAGIC 0x6D45774F // "OwEm"

save_file_s SAVEFILE;

typedef struct {
    ALIGNAS(16)
    u16 checksum;
    u8  version;  // save file version
    u8  platform; // platform used to produce this save file
    u32 magic;
    u8  unused[8];
} save_file_header_s;

const void *save_filename(i32 slot)
{
    switch (slot) {
    case 0: return "oe_save0.sav";
    case 1: return "oe_save1.sav";
    case 2: return "oe_save2.sav";
    }
    return 0;
}

err32 save_file_w_slot(save_file_s *sf, i32 slot)
{
    return save_file_w(sf, save_filename(slot));
}

err32 save_file_w(save_file_s *sf, const void *filename)
{
    if (!sf || !filename) return SAVE_FILE_ERR_BAD_ARG;

    void *f = pltf_file_open_w(filename);
    if (!f) return SAVE_FILE_ERR_OPEN;

    save_file_header_s h = {0};

    h.version  = SAVE_FILE_VERSION;
    h.platform = APP_VERSION_PLATFORM;
    h.magic    = SAVE_FILE_MAGIC;
    h.checksum = crc16_aligned4(0xFFFF, sf, sizeof(save_file_s));

    err32 err = 0;
    if (!pltf_file_w_checked(f, &h, sizeof(save_file_header_s)) ||
        !pltf_file_w_checked(f, sf, sizeof(save_file_s))) {
        err |= SAVE_FILE_ERR_RW;
    }
    if (!pltf_file_close(f)) {
        err |= SAVE_FILE_ERR_CLOSE;
    }
    return err;
}

err32 save_file_r_slot(save_file_s *sf, i32 slot)
{
    return save_file_r(sf, save_filename(slot));
}

// reads content from file and verifies checksum
static err32 save_file_r_buf(void *f, void *buf, usize buf_size, u32 checksum)
{
    if (!pltf_file_r_checked(f, buf, buf_size)) return SAVE_FILE_ERR_RW;
    return (checksum == crc16_aligned4(0xFFFF, buf, buf_size) ? 0 : SAVE_FILE_ERR_CHECKSUM);
}

err32 save_file_r(save_file_s *sf, const void *filename)
{
    if (!sf || !filename) return SAVE_FILE_ERR_BAD_ARG;

    void *f = pltf_file_open_r(filename);
    if (!f) return SAVE_FILE_ERR_OPEN;

    err32              err = 0;
    save_file_header_s h   = {0};
    if (pltf_file_r_checked(f, &h, sizeof(save_file_header_s)) && h.magic == SAVE_FILE_MAGIC) {
        allocator_s a_spm = spm_allocator();

        switch (h.version) {
        case SAVE_FILE_VERSION:
            err |= save_file_r_buf(f, sf, sizeof(save_file_s), h.checksum);
            break;
        default:
            err |= SAVE_FILE_ERR_VERSION;
            break;
        }
    } else {
        err |= SAVE_FILE_ERR_RW;
    }

    if (!pltf_file_close(f)) {
        err |= SAVE_FILE_ERR_CLOSE;
    }
    return err;
}
