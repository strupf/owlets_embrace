// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAMEDEF_H
#define GAMEDEF_H

#include "core/assets.h"
#include "core/aud.h"
#include "core/gfx.h"
#include "core/inp.h"
#include "core/spm.h"
#include "sys/sys.h"
#include "util/easing.h"
#include "util/json.h"
#include "util/mathfunc.h"
#include "util/mem.h"
#include "util/rng.h"
#include "util/str.h"

#define FILEPATH_MAP    "assets/map/"
#define FILEPATH_SND    "assets/snd/"
#define FILEPATH_MUS    "assets/mus/"
#define FILEPATH_TEX    "assets/tex/"
#define FILEPATH_FNT    "assets/fnt/"
#define FILEPATH_DIALOG "assets/dialog/"

typedef struct game_s game_s;
typedef struct obj_s  obj_s;

#define LEN_HERO_NAME          16
#define LEN_AREA_FILENAME      64
#define GAME_NUM_TILECOLLIDERS 32

enum {
    DIRECTION_NONE,
    DIRECTION_N,
    DIRECTION_NE,
    DIRECTION_E,
    DIRECTION_SE,
    DIRECTION_S,
    DIRECTION_SW,
    DIRECTION_W,
    DIRECTION_NW,
};

static int direction_nearest(int dir, bool32 cw)
{
    if (dir == DIRECTION_NONE) return 0;
    return ((dir + (cw ? 0 : 6)) & 7) + 1;
}

enum {
    TILELAYER_BG,
    TILELAYER_TERRAIN,
    //
    NUM_TILELAYER = 4
};

enum {
    GAMESTATE_MAINMENU,
    GAMESTATE_GAMEPLAY,
};

enum {
    GAME_STATE_NONE,
    GAME_STATE_TEXTBOX_IN,
    GAME_STATE_TEXTBOX,
    GAME_STATE_TEXTBOX_OUT,
    GAME_STATE_HERO_DIE,
    GAME_STATE_HERO_RESPAWN,
    GAME_STATE_ROOM_OUT,
    GAME_STATE_ROOM_BLACK,
    GAME_STATE_ROOM_IN,
    GAME_STATE_FREEZE,
    GAME_STATE_INTRO,
    GAME_STATE_MAINMENU,
};

enum {
    GAME_SUBSTATE_NONE,
    GAME_SUBSTATE_TEXTBOX_IN,
    GAME_SUBSTATE_TEXTBOX,
    GAME_SUBSTATE_TEXTBOX_OUT,
    GAME_SUBSTATE_HERO_DIE,
    GAME_SUBSTATE_HERO_RESPAWN,
    GAME_SUBSTATE_ROOM_OUT,
    GAME_SUBSTATE_ROOM_BLACK,
    GAME_SUBSTATE_ROOM_IN,
    GAME_SUBSTATE_FREEZE,
    GAME_SUBSTATE_INTRO,
    GAME_SUBSTATE_MAINMENU,
};

enum {
    TILE_EMPTY,
    TILE_BLOCK,
    //
    TILE_SLOPE_45,
    TILE_SLOPE_45_0 = TILE_SLOPE_45,
    TILE_SLOPE_45_1,
    TILE_SLOPE_45_2,
    TILE_SLOPE_45_3,
    //
    TILE_SLOPE_LO,
    TILE_SLOPE_LO_0 = TILE_SLOPE_LO,
    TILE_SLOPE_LO_1,
    TILE_SLOPE_LO_2,
    TILE_SLOPE_LO_3,
    TILE_SLOPE_LO_4,
    TILE_SLOPE_LO_5,
    TILE_SLOPE_LO_6,
    TILE_SLOPE_LO_7,

    //
    TILE_SLOPE_HI,
    TILE_SLOPE_HI_0 = TILE_SLOPE_HI,
    TILE_SLOPE_HI_1,
    TILE_SLOPE_HI_2,
    TILE_SLOPE_HI_3,
    TILE_SLOPE_HI_4,
    TILE_SLOPE_HI_5,
    TILE_SLOPE_HI_6,
    TILE_SLOPE_HI_7,
    //
    NUM_TILE_BLOCKS,
    //
    TILE_LADDER = NUM_TILE_BLOCKS,
    TILE_ONE_WAY,
    TILE_SPIKES,
};

static int ticks_from_seconds(f32 s)
{
    return (int)(s * (f32)SYS_UPS + .5f);
}

static f32 seconds_from_ticks(int ticks)
{
    return ((f32)ticks / (f32)SYS_UPS);
}

static int ticks_from_ms(int ms)
{
    return (ms * SYS_UPS + 500) / 1000;
}

static int ms_from_ticks(int ticks)
{
    return (ticks * 1000) / SYS_UPS;
}

static v2_i32 direction_v2(int dir)
{
    v2_i32 v = {0};
    switch (dir) {
    case DIRECTION_N: v.y = -1; break;
    case DIRECTION_S: v.y = +1; break;
    case DIRECTION_E: v.x = +1; break;
    case DIRECTION_W: v.x = -1; break;
    case DIRECTION_NE: v.y = -1, v.x = +1; break;
    case DIRECTION_SE: v.y = +1, v.x = +1; break;
    case DIRECTION_NW: v.y = -1, v.x = -1; break;
    case DIRECTION_SW: v.y = +1, v.x = -1; break;
    }
    return v;
}

static int ptr_index_in_arr(void *arr, void *p, int len)
{
    void **a = (void **)arr;
    for (int i = 0; i < len; i++) {
        if (*a++ == p) return i;
    }
    return -1;
}

enum {
    HITBOX_FLAG_HURT_HERO        = 1 << 0,
    HITBOX_FLAG_HURT_MONSTER     = 1 << 1,
    HITBOX_FLAG_HURT_ENVIRONMENT = 1 << 2,
    HITBOX_FLAG_HERO             = 1 << 3,
};

typedef struct {
    rec_i32 r;
    int     damage;
    int     flags;
    v2_i16  force_q8;
} hitbox_s;

#endif