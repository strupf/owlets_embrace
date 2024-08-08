// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SAVE_H
#define SAVE_H

#include "gamedef.h"

#define INVENTORY_NUM_ITEMS 128
#define NUM_MAP_PINS        64
#define NUM_SAVEIDS         256

typedef struct {
    u16 ID;
    u16 n;
} inventory_item_s;

typedef struct save_s {
    u32              tick;
    flags32          upgrades;
    u8               health;
    u8               flyupgrades;
    u8               n_map_pins;
    u8               n_items;
    u16              coins;
    u16              n_saveIDs;
    char             name[32];
    inventory_item_s items[INVENTORY_NUM_ITEMS];
    u32              saveIDs[NUM_SAVEIDS]; // unlocked things or events already happened
    char             hero_mapfile[64];
    char             areaname[64];
    v2_i32           hero_pos;
    map_pin_s        map_pins[NUM_MAP_PINS];
} save_s;

bool32 hero_has_upgrade(game_s *g, i32 ID);
void   hero_add_upgrade(game_s *g, i32 ID);
void   hero_rem_upgrade(game_s *g, i32 ID);
void   hero_set_name(game_s *g, const char *name);
char  *hero_get_name(game_s *g);
void   hero_inv_add(game_s *g, i32 ID, i32 n);
void   hero_inv_rem(game_s *g, i32 ID, i32 n);
i32    hero_inv_count_of(game_s *g, i32 ID);
void   hero_coins_change(game_s *g, i32 n);
i32    hero_coins(game_s *g);
void   saveID_put(game_s *g, u32 ID);
bool32 saveID_has(game_s *g, u32 ID);
//
void   savefile_empty(save_s *s);
bool32 savefile_read(i32 slot, save_s *s);
bool32 savefile_write(i32 slot, const save_s *s);
bool32 savefile_del(i32 slot);
bool32 savefile_cpy(i32 slot_from, i32 slot_to);

#endif