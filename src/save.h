// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SAVE_H
#define SAVE_H

#include "gamedef.h"
#include "minimap.h"

enum {
    SAVE_EV_NEW_GAME              = 1,
    SAVE_EV_INTRO_PLAYED          = 2,
    SAVE_EV_COMPANION_FOUND       = 3,
    SAVE_EV_UNLOCKED_MAP          = 4,
    SAVE_EV_CS_POWERUP_FIRST_TIME = 5,
    SAVE_EV_BOSS_GOLEM            = 200,
    SAVE_EV_BOSS_PLANT            = 201,
    SAVE_EV_BOSS_PLANT_INTRO_SEEN = 202,
    //
    NUM_SAVE_EV                   = 1024
};

b32 save_event_register(g_s *g, i32 ID);
b32 save_event_exists(g_s *g, i32 ID);

enum {
    SAVE_ERR_OPEN     = 1 << 0,
    SAVE_ERR_CLOSE    = 1 << 1,
    SAVE_ERR_RW       = 1 << 2,
    SAVE_ERR_VERSION  = 1 << 3,
    SAVE_ERR_CHECKSUM = 1 << 4,
};

typedef struct {
    ALIGNAS(8)
    u32 version;
    u16 checksum;
    u8  unused[2];
} save_header_s;

typedef struct {
    u32           tick;
    u8            name[LEN_HERO_NAME];
    u32           map_hash;
    v2_i16        hero_pos;
    u32           upgrades;
    u16           coins;
    u8            stamina;
    u8            stamina_pieces;
    u8            health;
    u8            health_max;
    u8            n_map_pins;
    u32           save[NUM_SAVE_EV / 32];
    minimap_pin_s pins[MAP_NUM_PINS];
    u32           map_visited[MINIMAP_N_SCREENS >> 5]; // 1 instead of 2 bits
} savefile_s;

// fills in empty/new savefile
void savefile_new(savefile_s *s, u8 *heroname);

b32 savefile_exists(i32 slot);

// writes the provided save to file; 0 on success
err32 savefile_w(i32 slot, savefile_s *s);

// tries to read a savefile into save; 0 on success
err32 savefile_r(i32 slot, savefile_s *s);
b32   savefile_del(i32 slot);
void  savefile_save_event_register(savefile_s *s, i32 ID);

#endif