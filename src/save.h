// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SAVE_H
#define SAVE_H

#include "gamedef.h"

enum {
    SAVEID_HARDCODE_RESERVED = 10000,
    //
    SAVEID_INTRO_PLAYED,
    SAVEID_UNLOCKED_MAP,
    SAVEID_BOSS_GOLEM = 10100,
};

#define NUM_MAP_PINS 64
#define NUM_SAVEIDS  256

typedef struct save_s {
    i32       tick; // 50 ticks/s -> 11930 h (for peace of mind)
    u32       upgrades;
    u32       charms;
    u8        stamina_upgrades;
    u8        n_map_pins;
    u8        name[16];
    u16       coins;
    u16       n_saveIDs;
    u16       saveIDs[NUM_SAVEIDS]; // unlocked things or events already happened
    u8        hero_mapfile[32];
    v2_i32    hero_pos;
    map_pin_s map_pins[NUM_MAP_PINS];
} save_s;

bool32 hero_has_upgrade(g_s *g, i32 ID);
void   hero_add_upgrade(g_s *g, i32 ID);
void   hero_rem_upgrade(g_s *g, i32 ID);
bool32 hero_has_charm(g_s *g, i32 ID);
void   hero_add_charm(g_s *g, i32 ID);
void   hero_set_name(g_s *g, const char *name);
char  *hero_get_name(g_s *g);
void   hero_inv_add(g_s *g, i32 ID, i32 n);
void   hero_inv_rem(g_s *g, i32 ID, i32 n);
i32    hero_inv_count_of(g_s *g, i32 ID);
void   hero_coins_change(g_s *g, i32 n);
i32    hero_coins(g_s *g);
i32    saveID_put(g_s *g, i32 ID);
bool32 saveID_has(g_s *g, i32 ID);
//
void   savefile_empty(save_s *s);
bool32 savefile_exists(i32 slot);
bool32 savefile_read(i32 slot, save_s *s);
bool32 savefile_write(i32 slot, const save_s *s);
bool32 savefile_del(i32 slot);
bool32 savefile_cpy(i32 slot_from, i32 slot_to);

#endif