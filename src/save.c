// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

err32 savefile_read_data(save_header_s h, void *f, void *buf, usize s);

void savefile_new(savefile_s *s, u8 *heroname)
{
    mclr(s, sizeof(savefile_s));
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

b32 savefile_exists(i32 slot)
{
    void *f = pltf_file_open_r(savefile_name(slot));
    if (f) {
        return (pltf_file_close(f) ? 1 : 2);
    }
    return 0;
}

err32 savefile_w(i32 slot, savefile_s *s)
{
    void *f = pltf_file_open_w(savefile_name(slot));
    if (!f) return SAVE_ERR_OPEN;

    err32         res = 0;
    save_header_s h   = {0};
    h.version         = GAME_VERSION;
    h.checksum        = crc16(s, sizeof(savefile_s));

    if (!pltf_file_w_checked(f, &h, sizeof(save_header_s)) ||
        !pltf_file_w_checked(f, s, sizeof(savefile_s))) {
        res |= SAVE_ERR_RW;
    }

    if (!pltf_file_close(f)) {
        res |= SAVE_ERR_CLOSE;
    }
    return res;
}

err32 savefile_r(i32 slot, savefile_s *s)
{
    void *f = pltf_file_open_r(savefile_name(slot));
    if (!f) return SAVE_ERR_OPEN;

    err32         res = 0;
    save_header_s h   = {0};
    if (pltf_file_r_checked(f, &h, sizeof(save_header_s))) {
        switch (h.version) {
        case GAME_VERSION: { // up to date version
            res |= savefile_read_data(h, f, s, sizeof(savefile_s));
            break;
        }
        default: { // unsupported
            res |= SAVE_ERR_VERSION;
            break;
        }
        }
    } else {
        res |= SAVE_ERR_RW;
    }

    if (!pltf_file_close(f)) {
        res |= SAVE_ERR_CLOSE;
    }
    return res;
}

b32 savefile_del(i32 slot)
{
    return pltf_file_del(savefile_name(slot));
}

err32 savefile_read_data(save_header_s h, void *f, void *buf, usize s)
{
    if (!pltf_file_r_checked(f, buf, s)) return SAVE_ERR_RW;

    if (h.checksum != crc16(buf, s)) {
        pltf_log("Mismatching save file checksum!\n");
        return SAVE_ERR_CHECKSUM;
    }
    return 0;
}

b32 save_event_register(g_s *g, i32 ID)
{
    if (0 < ID && ID < NUM_SAVE_EV && !save_event_exists(g, ID)) {
        g->save_events[ID >> 5] |= (u32)1 << (ID & 31);
        return 1;
    }
    return 0;
}

b32 save_event_exists(g_s *g, i32 ID)
{
    if (0 < ID && ID < NUM_SAVE_EV) {
        return (g->save_events[ID >> 5] & ((u32)1 << (ID & 31)));
    }
    return 0;
}