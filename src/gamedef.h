// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAMEDEF_H
#define GAMEDEF_H

#include "pltf/pltf.h"
//
#include "app_api.h"
#include "core/assets.h"
#include "core/aud.h"
#include "core/gfx.h"
#include "core/inp.h"
#include "core/spm.h"
#include "util/bitrw.h"
#include "util/easing.h"
#include "util/json.h"
#include "util/marena.h"
#include "util/mathfunc.h"
#include "util/rng.h"
#include "util/sorting.h"
#include "util/str.h"

#define GAME_DEMO  0
//
#define GAME_V_MAJ 0
#define GAME_V_MIN 2
#define GAME_V_PAT 1

#define GAME_VERSION_GEN(A, B, C) (((u32)(A) << 16) | \
                                   ((u32)(B) << 8) |  \
                                   ((u32)(C) << 0))

#define GAME_VERSION GAME_VERSION_GEN(GAME_V_MAJ, GAME_V_MIN, GAME_V_PAT)

typedef struct {
    ALIGNAS(4)
    u8 unused;
    u8 vmaj;
    u8 vmin;
    u8 vpat;
} game_version_s;

static game_version_s game_version_decode(u32 v)
{
    game_version_s r = {0xFF & (v >> 24),
                        0xFF & (v >> 16),
                        0xFF & (v >> 8),
                        0xFF & (v >> 0)};
    return r;
}

enum {
    TRIGGER_NULL                  = 0,
    //
    TRIGGER_CS_INTRO_COMP_1       = 1000,
    TRIGGER_CS_FINDING_COMP       = 1001,
    TRIGGER_CS_FINDING_HOOK       = 1002,
    //
    TRIGGER_TITLE_PREVIEW_TO_GAME = 2000,
    TRIGGER_BATTLEROOM_ENTER      = 2010,
    TRIGGER_BATTLEROOM_LEAVE      = 2011,
    TRIGGER_BOSS_PLANT            = 2200,
    //
    TRIGGER_DIA_FRAME_NEW         = 2500,
    TRIGGER_DIA_FRAME_END         = 2501,
    TRIGGER_DIA_END               = 2502,
    TRIGGER_DIA_CHOICE_1          = 2503,
    TRIGGER_DIA_CHOICE_2          = 2504,
    TRIGGER_DIA_CHOICE_3          = 2505,
    TRIGGER_DIA_CHOICE_4          = 2506,
};

enum {
    SAVE_EV_UNLOCKED_MAP          = 2,
    SAVE_EV_COMPANION_FOUND       = 3,
    SAVE_EV_CS_POWERUP_FIRST_TIME = 5,
    SAVE_EV_CS_INTRO_COMP_1       = 6, // companion hushing through the tutorial area #1
    SAVE_EV_CS_HOOK_FOUND         = 7,
    SAVE_EV_CRACKBLOCK_INTRO_1    = 8,
    SAVE_EV_PUSHBLOCK_INTRO_1     = 9,
    SAVE_EV_BOSS_GOLEM            = 200,
    SAVE_EV_BOSS_PLANT            = 201,
    SAVE_EV_BOSS_PLANT_INTRO_SEEN = 202,
    //
    NUM_SAVE_EV
};

enum {
    MUSIC_ID_NONE,
    MUSIC_ID_CAVE,
    MUSIC_ID_WATERFALL,
    MUSIC_ID_SNOW,
    MUSIC_ID_FOREST,
    MUSIC_ID_ANCIENT_TREE,
    MUSIC_ID_INTRO,
};

enum {
    AREA_ID_NONE,
    AREA_ID_DEEP_FOREST,
    AREA_ID_CAVE,
    AREA_ID_SNOW_PEAKS,
    AREA_ID_MOUNTAIN,
};

#define MAP_WAD_NAME_LEN 16
#define OWL_LEN_NAME     20

typedef struct g_s   g_s;
typedef struct obj_s obj_s;

// handle to an object
// object pointer is valid (still exists) if:
//   o != NULL && GID == o->GID
typedef struct obj_handle_s {
    ALIGNAS(8)
    obj_s *o;
    u32    generation;
} obj_handle_s;

enum {
    DIR_NONE  = 0,
    DIR_X_POS = 1 << 1,
    DIR_X_NEG = 1 << 2,
    DIR_Y_POS = 1 << 3,
    DIR_Y_NEG = 1 << 4,
    //
    DIR_N     = DIR_Y_NEG,
    DIR_S     = DIR_Y_POS,
    DIR_W     = DIR_X_NEG,
    DIR_E     = DIR_X_POS,
    DIR_NW    = DIR_Y_NEG | DIR_X_NEG,
    DIR_NE    = DIR_Y_NEG | DIR_X_POS,
    DIR_SW    = DIR_Y_POS | DIR_X_NEG,
    DIR_SE    = DIR_Y_POS | DIR_X_POS,
    //
    DIR_UP    = DIR_Y_NEG,
    DIR_DOWN  = DIR_Y_POS,
    DIR_LEFT  = DIR_X_NEG,
    DIR_RIGHT = DIR_X_POS,
};

static i32 dir_nswe_index(i32 i)
{
    switch (i) {
    case 1: return DIR_N;
    case 2: return DIR_NE;
    case 3: return DIR_E;
    case 4: return DIR_SE;
    case 5: return DIR_S;
    case 6: return DIR_SW;
    case 7: return DIR_W;
    case 8: return DIR_NW;
    }
    return 0;
}

static i32 dir_nswe_nearest(i32 dir, bool32 cw)
{
    switch (dir) {
    case DIR_N: return (cw ? DIR_NE : DIR_NW);
    case DIR_S: return (cw ? DIR_SW : DIR_SE);
    case DIR_E: return (cw ? DIR_SE : DIR_NE);
    case DIR_W: return (cw ? DIR_NW : DIR_SW);
    case DIR_NW: return (cw ? DIR_N : DIR_W);
    case DIR_NE: return (cw ? DIR_E : DIR_N);
    case DIR_SW: return (cw ? DIR_W : DIR_S);
    case DIR_SE: return (cw ? DIR_S : DIR_E);
    }
    return 0;
}

static i32 dir_nswe_90_deg(i32 dir, bool32 cw)
{
    i32 d = dir;
    d     = dir_nswe_nearest(d, cw);
    d     = dir_nswe_nearest(d, cw);
    return d;
}

static i32 dir_nswe_opposite(i32 dir)
{
    i32 d = dir;
    if (0) {
    } else if (d & DIR_X_POS) {
        d = (d & ~DIR_X_POS) | DIR_X_NEG;
    } else if (d & DIR_X_NEG) {
        d = (d & ~DIR_X_NEG) | DIR_X_POS;
    }
    if (0) {
    } else if (d & DIR_Y_POS) {
        d = (d & ~DIR_Y_POS) | DIR_Y_NEG;
    } else if (d & DIR_Y_NEG) {
        d = (d & ~DIR_Y_NEG) | DIR_Y_POS;
    }
    return d;
}

static v2_i32 dir_v2(i32 dir)
{
    v2_i32 v = {0};
    if (dir & DIR_X_POS) v.x = +1;
    if (dir & DIR_X_NEG) v.x = -1;
    if (dir & DIR_Y_POS) v.y = +1;
    if (dir & DIR_Y_NEG) v.y = -1;
    return v;
}

static i32 ticks_from_seconds(f32 s)
{
    return (i32)(s * (f32)PLTF_UPS + .5f);
}

static f32 seconds_from_ticks(i32 ticks)
{
    return ((f32)ticks / (f32)PLTF_UPS);
}

static i32 ticks_from_ms(i32 ms)
{
    return (ms * PLTF_UPS + 500) / 1000;
}

static i32 ms_from_ticks(i32 ticks)
{
    return (ticks * 1000) / PLTF_UPS;
}

static i32 find_ptr_in_array(void *arr, void *p, i32 len)
{
    void **a = (void **)arr;
    for (i32 i = 0; i < len; i++) {
        if (*a++ == p) return i;
    }
    return -1;
}

enum {
    HITBOX_FLAG_POWERSTOMP = 1 << 0,
};

typedef struct hitbox_legacy_s {
    ALIGNAS(32)
    rec_i32 r;
    v2_i16  force_q8;
    u8      damage;
    u8      flags;
    u8      hitID;
} hitbox_legacy_s;

typedef struct time_real_s {
    ALIGNAS(8)
    u16 h;
    u16 m;
    u16 s;
    u16 ms;
} time_real_s;

#define TIME_K_S ((u32)PLTF_UPS)
#define TIME_K_M ((u32)60 * TIME_K_S)
#define TIME_K_H ((u32)60 * TIME_K_M)

static time_real_s time_real_from_ticks(u32 t)
{
    time_real_s time = {
        ((t / TIME_K_H)),                   // h
        ((t % TIME_K_H) / TIME_K_M),        // m
        ((t % TIME_K_M) / TIME_K_S),        // s
        ((t % TIME_K_S) * 1000) / TIME_K_S, // ms
    };
    return time;
}

static u32 ticks_from_time_real(time_real_s t)
{
    return ((u32)t.h * TIME_K_H) +
           ((u32)t.m * TIME_K_M) +
           ((u32)t.s * TIME_K_S) +
           ((u32)t.ms * TIME_K_S) / 1000;
}

// maps time t to a frame number [0; frames) at a looping frequence of freq
static inline i32 frame_from_ticks_loop(u32 t, u32 frames)
{
    return (t % frames);
}

// maps time t to a frame number [0; frames), looping back and forth
// one loop is 0 to frames - 1 and back to 0
static i32 frame_from_ticks_pingpong(i32 t, i32 frames)
{
    i32 n = (frames << 1) - 2;
    i32 k = modu_i32(t, n);
    return (frames <= k ? n - k : k);
}

#endif