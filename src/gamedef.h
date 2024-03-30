// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAMEDEF_H
#define GAMEDEF_H

#define GAME_VERSION_MAJ 0
#define GAME_VERSION_MIN 1

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

#define SAVEFILE_NAME   "save.sav"
#define FILEPATH_MAP    "assets/map/"
#define FILEPATH_SND    "assets/snd/"
#define FILEPATH_MUS    "assets/mus/"
#define FILEPATH_TEX    "assets/tex/"
#define FILEPATH_FNT    "assets/fnt/"
#define FILEPATH_DIALOG "assets/dialog/"

typedef struct game_s game_s;
typedef struct obj_s  obj_s;

enum {
    APP_STATE_TITLE,
    APP_STATE_GAME,
};

enum {
    GAME_STATE_PLAY,
    GAME_STATE_DIALOG,
    GAME_STATE_MENU,
    GAME_STATE_SETTINGS,
    GAME_STATE_GAMEOVER,
};

#define LEN_HERO_NAME             16
#define LEN_AREA_FILENAME         64
#define GAME_NUM_TILECOLLIDERS    32
#define FADETICKS_MM_GAME         40
#define FADETICKS_MM_GAME_BLACK   20
#define FADETICKS_GAME_IN         40
#define FADETICKS_AREALABEL       150
#define RENDER_PRIO_HERO          0x100
#define ITEM_SWAP_DOUBLETAP_TICKS 15
#define NUM_WORLD_ROOMS           64
#define NUM_WORLD_ROOM_TILES      256

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

static i32 time_now()
{
    return (i32)sys_tick();
}

static i32 time_since(i32 t)
{
    i32 time = time_now();
    return (time - t);
}

enum {
    TILELAYER_BG,
    TILELAYER_PROP_BG,
    TILELAYER_PROP_FG,
    //
    NUM_TILELAYER
};

enum {
    MENUITEM_GAME_INVENTORY,
    MENUITEM_REDUCE_FLICKER,
};

enum {
    TILE_TYPE_NONE,
    TILE_TYPE_FAKE_1,
    TILE_TYPE_FAKE_2,
    //
    TILE_TYPE_CLEAN       = 3,
    TILE_TYPE_BRICK       = 4,
    TILE_TYPE_BRICK_SMALL = 5,
    TILE_TYPE_DIRT        = 6,
    TILE_TYPE_STONE       = 7,
    TILE_TYPE_1           = 8,
    TILE_TYPE_2           = 9,
    TILE_TYPE_3           = 10,
    TILE_TYPE_DIRT_DARK   = 11,
    //
    NUM_TILE_TYPES
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
    TILE_LADDER_ONE_WAY,
    TILE_ONE_WAY,
    TILE_SPIKES,
};

static i32 ticks_from_seconds(f32 s)
{
    return (i32)(s * (f32)SYS_UPS + .5f);
}

static f32 seconds_from_ticks(i32 ticks)
{
    return ((f32)ticks / (f32)SYS_UPS);
}

static i32 ticks_from_ms(i32 ms)
{
    return (ms * SYS_UPS + 500) / 1000;
}

static i32 ms_from_ticks(i32 ticks)
{
    return (ticks * 1000) / SYS_UPS;
}

static v2_i32 direction_v2(i32 dir)
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
    default: break;
    }
    return v;
}

static i32 ptr_index_in_arr(void *arr, void *p, i32 len)
{
    void **a = (void **)arr;
    for (i32 i = 0; i < len; i++) {
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

typedef struct {
    u32    type;
    v2_i32 pos;
} map_pin_s;

typedef struct {
    i32 x;
} settings_s;

#endif