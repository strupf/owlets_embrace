// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SAVE_FILE_H
#define SAVE_FILE_H

#define SAVE_FILE_VERSION 0

#include "gamedef.h"
#include "minimap.h"
#include "pltf/pltf_types.h"

enum {
    SAVE_FILE_ERR_BAD_ARG  = 1 << 0,
    SAVE_FILE_ERR_OPEN     = 1 << 1,
    SAVE_FILE_ERR_RW       = 1 << 2,
    SAVE_FILE_ERR_VERSION  = 1 << 3,
    SAVE_FILE_ERR_CHECKSUM = 1 << 5,
    SAVE_FILE_ERR_CLOSE    = 1 << 6,
    SAVE_FILE_ERR_MISC     = 1 << 7,
};

typedef struct {
    ALIGNAS(32)
    u32           tick;
    u8            name[OWL_LEN_NAME];
    u8            map_name[MAP_WAD_NAME_LEN]; // name of map in editor
    v2_i16        hero_pos;
    i32           owl_x;
    i32           owl_y;
    u32           upgrades;
    u32           coins;
    u32           stamina;
    u32           stamina_pieces;
    u32           health_pieces;
    u32           health;
    u32           health_max;
    u32           n_map_pins;
    u32           save[NUM_SAVEID_WORDS];
    minimap_pin_s pins[MAP_NUM_PINS];
    u32           map_visited[MINIMAP_N_SCREENS >> 5]; // 1 bit per visited screen tile
    //
    u32           unused[64]; // reserved for possible additions in the future
} save_file_s;

extern save_file_s SAVEFILE;

const void *save_filename(i32 slot);
err32       save_file_r_slot(save_file_s *sf, i32 slot);
err32       save_file_w_slot(save_file_s *sf, i32 slot);
err32       save_file_r(save_file_s *sf, const void *filename);
err32       save_file_w(save_file_s *sf, const void *filename);

#endif