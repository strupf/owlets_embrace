// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SAVE_H
#define SAVE_H

#include "gamedef.h"
#include "minimap.h"
#include "objdef.h"

enum {
    SAVEID_UNLOCKED_MAP          = 2,
    SAVEID_COMPANION_FOUND       = 3,
    SAVEID_CS_POWERUP_FIRST_TIME = 5,
    SAVEID_CS_INTRO_COMP_1       = 6, // companion hushing through the tutorial area #1
    SAVEID_CS_HOOK_FOUND         = 7,
    SAVEID_CRACKBLOCK_INTRO_1    = 8,
    SAVEID_PUSHBLOCK_INTRO_1     = 9,
    SAVEID_BOSS_PLANT            = 200,
    SAVEID_BOSS_PLANT_INTRO_SEEN = 201,
    //
    NUM_SAVEIDS                  = 1024
};

#define NUM_SAVEID_WORDS ((NUM_SAVEIDS + 31) >> 5)

void saveID_put(g_s *g, i32 ID);
b32  saveID_has(g_s *g, i32 ID);

enum {
    SAVE_ERR_OPEN     = 1 << 0,
    SAVE_ERR_CLOSE    = 1 << 1,
    SAVE_ERR_RW       = 1 << 2,
    SAVE_ERR_VERSION  = 1 << 3,
    SAVE_ERR_CHECKSUM = 1 << 4,
};

typedef struct {
    ALIGNAS(32)
    u32           tick;
    u8            name[OWL_LEN_NAME];
    u8            map_name[MAP_WAD_NAME_LEN]; // name of map in editor
    v2_i16        hero_pos;
    u32           upgrades;
    u16           coins;
    u8            stamina;
    u8            stamina_pieces;
    u8            health_pieces;
    u8            health;
    u8            health_max;
    u8            n_map_pins;
    u16           enemy_killed[NUM_ENEMYID];
    u32           save[NUM_SAVEID_WORDS];
    minimap_pin_s pins[MAP_NUM_PINS];
    u32           map_visited[MINIMAP_N_SCREENS >> 5]; // 1 instead of 2 bits
} savefile_s;

void  savefile_new(savefile_s *s, u8 *heroname); // fills in empty/new savefile
b32   savefile_exists(i32 slot);
err32 savefile_w(i32 slot, savefile_s *s); // writes the provided save to file; 0 on success
err32 savefile_r(i32 slot, savefile_s *s); // tries to read a savefile into save; 0 on success
b32   savefile_del(i32 slot);
void  savefile_saveID_put(savefile_s *s, i32 ID);

#endif