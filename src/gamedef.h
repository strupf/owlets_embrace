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
    DIRECTION_W = 1 << 0,
    DIRECTION_N = 1 << 1,
    DIRECTION_E = 1 << 2,
    DIRECTION_S = 1 << 3,
};

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
    if (dir & DIRECTION_E) v.x = +1;
    if (dir & DIRECTION_W) v.x = -1;
    if (dir & DIRECTION_S) v.y = +1;
    if (dir & DIRECTION_N) v.y = -1;
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

typedef union {
    u32 i[4];
    u8  b[16];
} GUID_s;

// AAAAAAAA BBBB BBBB CCCC CCCCDDDDDDDD
// cb422400-6280-11ee-b652-81974dacc861
static GUID_s GUID_parse_str(const char *str)
{
    GUID_s      g = {0};
    int         b = 0;
    const char *s = str;
    while (b < 16) {
        if (!char_is_xdigit(*s)) {
            s++;
            continue;
        }
        g.b[b++] = (char_int_from_hex(s[0])) | (char_int_from_hex(s[1])) << 4;
        s += 2;
    }
    return g;
}

static bool32 GUID_same(GUID_s a, GUID_s b)
{
    return (a.i[0] == b.i[0] && a.i[1] == b.i[1] &&
            a.i[2] == b.i[2] && a.i[3] == b.i[3]);
}

enum {
    HITBOX_FLAG_HURT_HERO        = 1 << 0,
    HITBOX_FLAG_HURT_MONSTER     = 1 << 1,
    HITBOX_FLAG_HURT_ENVIRONMENT = 1 << 2,
};

typedef struct {
    rec_i32 r;
    int     damage;
    int     flags;
} hitbox_s;

#endif