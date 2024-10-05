// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

bool32 hero_has_upgrade(game_s *g, i32 ID)
{
    save_s *hs = &g->save;
    return (hs->upgrades & ((flags32)1 << ID));
}

void hero_add_upgrade(game_s *g, i32 ID)
{
    save_s *hs = &g->save;
    hs->upgrades |= (flags32)1 << ID;
    switch (ID) {
    case HERO_UPGRADE_FLY: {
        hs->flyupgrades = 2;
        break;
    case HERO_UPGRADE_SWIM: {
        hs->upgrades |= (flags32)1 << HERO_UPGRADE_DIVE;
        break;
    }
    }
    }
}

void hero_rem_upgrade(game_s *g, i32 ID)
{
    save_s *hs = &g->save;
    hs->upgrades &= ~((flags32)1 << ID);
}

void hero_set_name(game_s *g, const char *name)
{
    save_s *hs = &g->save;
    str_cpy(hs->name, name);
}

char *hero_get_name(game_s *g)
{
    save_s *hs = &g->save;
    return &hs->name[0];
}

void hero_inv_add(game_s *g, i32 ID, i32 n)
{
    save_s *hs = &g->save;
    hs->items[ID].n += n;
}

void hero_inv_rem(game_s *g, i32 ID, i32 n)
{
    save_s *hs = &g->save;
    assert(n <= hero_inv_count_of(g, ID));
    hs->items[ID].n -= n;
}

i32 hero_inv_count_of(game_s *g, i32 ID)
{
    save_s *hs = &g->save;
    return hs->items[ID].n;
}

void hero_coins_change(game_s *g, i32 n)
{
    if (n == 0) return;

    i32 ct = g->save.coins + g->coins_added + n;
    if (ct < 0) return;

    if (g->coins_added == 0 || g->coins_added_ticks) {
        g->coins_added_ticks = 100;
    }
    g->coins_added += n;
}

i32 hero_coins(game_s *g)
{
    i32 c = g->save.coins + g->coins_added;
    assert(0 <= c);
    return c;
}

i32 saveID_put(game_s *g, u32 ID)
{
    save_s *hs = &g->save;
    if (saveID_has(g, ID)) return 2;
    if (hs->n_saveIDs == NUM_SAVEIDS) return 0;
    hs->saveIDs[hs->n_saveIDs++] = ID;
    return 1;
}

bool32 saveID_has(game_s *g, u32 ID)
{
    save_s *hs = &g->save;
    for (i32 n = 0; n < hs->n_saveIDs; n++) {
        if (hs->saveIDs[n] == ID) return 1;
    }
    return 0;
}

void savefile_empty(save_s *s)
{
    mset(s, 0, sizeof(save_s));
    s->health = 3;
    str_cpy(s->hero_mapfile, "L_0");
}

static inline const char *savefile_name(i32 slot)
{
    switch (slot) {
    case 0: return "save_0.sav";
    case 1: return "save_1.sav";
    case 2: return "save_2.sav";
    }
    return NULL;
}

bool32 savefile_read(i32 slot, save_s *s)
{
    void *f = pltf_file_open_r(savefile_name(slot));

    if (!f) return 0;
    if (!s) {
        pltf_file_close(f);
        return 1;
    }

    u32 ver = 0;
    pltf_file_r(f, &ver, sizeof(u32)); // convert savefiles with old version
    switch (ver) {
    default:
        pltf_file_r(f, s, sizeof(save_s));
        break;
    }
    pltf_file_close(f);
    return 1;
}

bool32 savefile_write(i32 slot, const save_s *s)
{
    void *f = pltf_file_open_w(savefile_name(slot));

    if (!f) return 0;
    u32 ver = GAME_VERSION;
    pltf_file_w(f, &ver, sizeof(u32));
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
    save_s sav = {0};
    if (!savefile_read(slot_from, &sav)) return 0;
    return savefile_write(slot_to, &sav);
}