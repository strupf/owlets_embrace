// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAME_DEF_H
#define GAME_DEF_H

#include "os/os.h"

#define GAME_HEAPMEM          0x40000
#define NUM_TILES             1048576
#define ASSET_PATH_MAPS       "assets/map/"
#define ASSET_PATH_DIALOGUE   "assets/"
#define NUM_OBJS              256
#define LEN_STR_AREA_FILENAME 64
#define LEN_STR_PLAYER_NAME   16
#define INTERACTABLE_DIST     100

typedef struct objhandle_s  objhandle_s;
typedef struct obj_s        obj_s;
typedef struct room_s       room_s;
typedef struct objset_s     objset_s;
typedef struct obj_listc_s  obj_listc_s;
typedef struct objbucket_s  objbucket_s;
typedef struct game_s       game_s;
typedef struct textbox_s    textbox_s;
typedef struct ropenode_s   ropenode_s;
typedef struct rope_s       rope_s;
typedef struct transition_s transition_s;
typedef struct cam_s        cam_s;

enum {
        AREA_NAME_DISPLAY_TICKS = 100,
        AREA_NAME_FADE_TICKS    = (AREA_NAME_DISPLAY_TICKS * 1) / 4,
};

#define NUM_AUTOTILE_MAIN  16
#define NUM_AUTOTILE_BG    48
#define NUM_AUTOTILE_TYPES (NUM_AUTOTILE_BG + NUM_AUTOTILE_MAIN)

enum {
        TMJ_TILESET_FGID = NUM_AUTOTILE_TYPES * 16 + 1, // 16 in a row per tiletype
};

#define foreach_tile_in_bounds(X1, Y1, X2, Y2, XIT, YIT) \
        for (int YIT = Y1; YIT <= Y2; YIT++)             \
                for (int XIT = X1; XIT <= X2; XIT++)

enum {
        GAMESTATE_GAMEPLAY,
        GAMESTATE_TITLE,
};

enum {
        TILE_LAYER_MAIN,
        TILE_LAYER_BG
};

enum {
        DIRECTION_NONE,
        DIRECTION_W,
        DIRECTION_N,
        DIRECTION_E,
        DIRECTION_S,
};

enum {
        TILE_EMPTY      = 0,
        TILE_BLOCK      = 1,
        //
        TILE_SLOPE_45   = 2,
        TILE_SLOPE_45_1 = TILE_SLOPE_45,
        TILE_SLOPE_45_2,
        TILE_SLOPE_45_3,
        TILE_SLOPE_45_4,
        //
        TILE_SLOPE_LO   = 6,
        TILE_SLOPE_LO_1 = TILE_SLOPE_LO,
        TILE_SLOPE_LO_2,
        TILE_SLOPE_LO_3,
        TILE_SLOPE_LO_4,
        TILE_SLOPE_LO_5,
        TILE_SLOPE_LO_6,
        TILE_SLOPE_LO_7,
        TILE_SLOPE_LO_8,
        //
        TILE_SLOPE_HI   = 14,
        TILE_SLOPE_HI_1 = TILE_SLOPE_HI,
        TILE_SLOPE_HI_2,
        TILE_SLOPE_HI_3,
        TILE_SLOPE_HI_4,
        TILE_SLOPE_HI_5,
        TILE_SLOPE_HI_6,
        TILE_SLOPE_HI_7,
        TILE_SLOPE_HI_8,
        //
        NUM_TILE_BLOCKS,
        TILE_LADDER,
};

#endif