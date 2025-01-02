// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

bool32 hero_has_upgrade(g_s *g, i32 ID)
{
    save_s *hs = &g->save;
    return (hs->upgrades & ((u32)1 << ID));
}

void hero_add_upgrade(g_s *g, i32 ID)
{
    save_s *hs = &g->save;
    hs->upgrades |= (u32)1 << ID;
    pltf_log("# ADD UPGRADE: %i\n", ID);
}

void hero_rem_upgrade(g_s *g, i32 ID)
{
    save_s *hs = &g->save;
    hs->upgrades &= ~((u32)1 << ID);
    pltf_log("# DEL UPGRADE: %i\n", ID);
}

bool32 hero_has_charm(g_s *g, i32 ID)
{
    save_s *hs = &g->save;
    return (hs->charms & ((u32)1 << ID));
}

void hero_add_charm(g_s *g, i32 ID)
{
    save_s *hs = &g->save;
    hs->charms |= (u32)1 << ID;
}

void hero_set_name(g_s *g, const char *name)
{
    save_s *hs = &g->save;
    str_cpy(hs->name, name);
}

char *hero_get_name(g_s *g)
{
    save_s *hs = &g->save;
    return &hs->name[0];
}

void hero_inv_add(g_s *g, i32 ID, i32 n)
{
    save_s *hs = &g->save;
}

void hero_inv_rem(g_s *g, i32 ID, i32 n)
{
    save_s *hs = &g->save;
}

i32 hero_inv_count_of(g_s *g, i32 ID)
{
    save_s *hs = &g->save;
    return 0;
}

void hero_coins_change(g_s *g, i32 n)
{
    if (n == 0) return;

    i32 ct = g->save.coins + g->coins_added + n;
    if (ct < 0) return;

    if (g->coins_added == 0 || g->coins_added_ticks) {
        g->coins_added_ticks = 100;
    }
    g->coins_added += n;
}

i32 hero_coins(g_s *g)
{
    i32 c = g->save.coins + g->coins_added;
    assert(0 <= c);
    return c;
}

i32 saveID_put(g_s *g, i32 ID)
{
    if (ID == 0) return 0;

    save_s *hs = &g->save;
    if (saveID_has(g, ID)) return 2;
    if (hs->n_saveIDs == NUM_SAVEIDS) return 0;
    hs->saveIDs[hs->n_saveIDs++] = ID;
    return 1;
}

bool32 saveID_has(g_s *g, i32 ID)
{
    if (ID == 0) return 0;

    save_s *hs = &g->save;
    for (i32 n = 0; n < hs->n_saveIDs; n++) {
        if (hs->saveIDs[n] == ID) return 1;
    }
    return 0;
}

void savefile_empty(save_s *s)
{
    mset(s, 0, sizeof(save_s));
    str_cpy(s->hero_mapfile, "L_0");
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
        pltf_file_close(f);
        return 1;
    }
    return 0;
}

bool32 savefile_read(i32 slot, save_s *s)
{
    if (!s) return 0;

    void *f = pltf_file_open_r(savefile_name(slot));
    if (!f) return 0;

    bool32 res     = 1;
    u32    version = 0;
    pltf_file_r(f, &version, sizeof(u32));

    switch (version) {
    case GAME_VERSION:
        pltf_file_r(f, s, sizeof(save_s));
        break;
    default: // reading unsupported savefile
        res = 0;
        break;
    }

    pltf_file_close(f);
    return res;
}

bool32 savefile_write(i32 slot, const save_s *s)
{
    if (!s) return 0;

    void *f = pltf_file_open_w(savefile_name(slot));
    if (!f) return 0;

    u32 version = GAME_VERSION;
    pltf_file_w(f, &version, sizeof(u32));
    pltf_file_w(f, s, sizeof(save_s));
    pltf_file_close(f);
    return 1;
}

bool32 savefile_del(i32 slot)
{
    return pltf_file_del(savefile_name(slot));
}

bool32 savefile_cpy(i32 slot_from, i32 slot_to)
{
    spm_push();
    bool32  res = 0;
    save_s *s   = spm_alloct(save_s, 1);
    if (savefile_read(slot_from, s)) {
        savefile_write(slot_to, s);
        res = 1;
    }
    spm_pop();
    return res;
}