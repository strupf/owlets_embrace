// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    u32 version;
    u32 checksum;
    u8  ununsed[8];
} save_header_s;

static_assert(sizeof(save_header_s) == 16, "save header size");

void savefile_upgrade(const void *s_old, u32 v, save_s *s);
i32  savefile_read_data(void *f, void *buf, usize s, u32 checksum);

void savefile_new(save_s *s, u8 *heroname)
{
    mclr(s, sizeof(save_s));
    str_cpys(s->name, sizeof(s->name), heroname);
}

static inline const char *savefile_name(i32 slot)
{
    switch (slot) {
    case 0: return "save_0.bin";
    case 1: return "save_1.bin";
    case 2: return "save_2.bin";
    }
    return 0;
}

bool32 savefile_exists(i32 slot)
{
    void *f = pltf_file_open_r(savefile_name(slot));
    if (f) {
        return (pltf_file_close(f) ? 1 : 2);
    }
    return 0;
}

i32 savefile_w(i32 slot, save_s *s)
{
    void *f = pltf_file_open_w(savefile_name(slot));
    if (!f) return SAVE_ERR_OPEN;

    i32           res = 0;
    save_header_s h   = {GAME_VERSION,
                         checksum_u32(s, sizeof(save_s))};
    if (!pltf_file_ws(f, &h, sizeof(save_header_s)) ||
        !pltf_file_ws(f, s, sizeof(save_s))) {
        res |= SAVE_ERR_RW;
    }

    if (!pltf_file_close(f)) {
        res |= SAVE_ERR_CLOSE;
    }
    return res;
}

i32 savefile_r(i32 slot, save_s *s)
{
    void *f = pltf_file_open_r(savefile_name(slot));
    if (!f) return SAVE_ERR_OPEN;

    i32           res = 0;
    save_header_s h   = {0};
    if (!pltf_file_rs(f, &h, sizeof(save_header_s))) {
        res |= SAVE_ERR_RW;
        goto CLOSEF;
    }

    switch (h.version) {
    case GAME_VERSION: {
        // up to date version
        res |= savefile_read_data(f, s, sizeof(save_s), h.checksum);
        break;
    }
#if 0
        // future: update old save files
    case SOME_OLD_VERSION: {
        // read data
        savefile_upgrade(old_data, h.version, s);
        break;
    }
#endif
    default: {
        // unsupported
        res |= SAVE_ERR_VERSION;
        i32 major, minor, patch;
        game_version_decode(h.version, &major, &minor, &patch);

        pltf_log("Incompatible savefile version!\n");
        pltf_log("Game: %i.%i.%i\n", GAME_V_MAJOR, GAME_V_MINOR, GAME_V_PATCH);
        pltf_log("Save: %i.%i.%i\n", major, minor, patch);
        break;
    }
    }

CLOSEF:;
    if (!pltf_file_close(f)) {
        res |= SAVE_ERR_CLOSE;
    }
    return res;
}

bool32 savefile_del(i32 slot)
{
    return pltf_file_del(savefile_name(slot));
}

bool32 savefile_cpy(i32 slot_from, i32 slot_to)
{
    spm_push();
    save_s *s = spm_alloct(save_s, 1);
    b32     r =
        savefile_r(slot_from, s) == 0 &&
        savefile_w(slot_to, s) == 0;
    spm_pop();
    return r;
}

i32 savefile_read_data(void *f, void *buf, usize s, u32 checksum)
{
    if (!pltf_file_rs(f, buf, s))
        return SAVE_ERR_RW;

    if (checksum != checksum_u32(buf, s)) {
        pltf_log("Mismatching save file checksum!\n");
        return SAVE_ERR_CHECKSUM;
    }
    return 0;
}

void savefile_upgrade(const void *s_old, u32 v, save_s *s)
{
}
