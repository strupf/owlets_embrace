// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SAVE_H
#define SAVE_H

#include "gamedef.h"
#include "map.h"

enum {
    SAVE_EV_RESERVED = 3000,
    //
    SAVE_EV_NEW_GAME,
    SAVE_EV_INTRO_PLAYED,
    SAVE_EV_UNLOCKED_MAP,
    //
    SAVE_EV_BOSS_GOLEM = 3100,
    //
    NUM_SAVE_EV        = 4096
};

i32    save_event_register(g_s *g, i32 ID);
bool32 save_event_exists(g_s *g, i32 ID);

enum {
    SAVE_ERR_OPEN     = 1 << 0,
    SAVE_ERR_CLOSE    = 1 << 1,
    SAVE_ERR_RW       = 1 << 2,
    SAVE_ERR_VERSION  = 1 << 3,
    SAVE_ERR_CHECKSUM = 1 << 4,
};

typedef struct {
    u32       tick;
    u8        name[LEN_HERO_NAME];
    u32       map_hash;
    v2_i16    hero_pos;
    u32       upgrades;
    u32       enemies_killed;
    u8        stamina;
    u16       pos_x;
    u16       pos_y;
    u16       coins;
    u16       n_map_pins;
    u32       save[NUM_SAVE_EV / 32];
    map_pin_s pins[MAP_NUM_PINS];
} savefile_s;

// fills in empty/new savefile
void savefile_new(savefile_s *s, u8 *heroname);

bool32 savefile_exists(i32 slot);

// writes the provided save to file; 0 on success
i32 savefile_w(i32 slot, savefile_s *s);

// tries to read a savefile into save; 0 on success
i32    savefile_r(i32 slot, savefile_s *s);
bool32 savefile_del(i32 slot);
bool32 savefile_cpy(i32 slot_from, i32 slot_to);

#endif