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

enum {
    SAVE_ERR_OPEN     = 1 << 0,
    SAVE_ERR_CLOSE    = 1 << 1,
    SAVE_ERR_RW       = 1 << 2,
    SAVE_ERR_VERSION  = 1 << 3,
    SAVE_ERR_CHECKSUM = 1 << 4,
};

typedef struct save_s {
    u32    tick;
    u8     name[LEN_HERO_NAME];
    u32    map_hash;
    v2_i16 hero_pos;
    u32    upgrades;
    u8     stamina;
    u16    pos_x;
    u16    pos_y;
    u16    coins;
    u16    n_saveIDs;
    u16    saveIDs[NUM_SAVEIDS];
} save_s;

// fills in empty/new savefile
void savefile_new(save_s *s, u8 *heroname);

bool32 savefile_exists(i32 slot);

// writes the provided save to file; 0 on success
i32 savefile_w(i32 slot, save_s *s);

// tries to read a savefile into save; 0 on success
i32    savefile_r(i32 slot, save_s *s);
bool32 savefile_del(i32 slot);
bool32 savefile_cpy(i32 slot_from, i32 slot_to);

#endif