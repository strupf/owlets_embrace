// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAMEDEF_H
#define GAMEDEF_H

#include "pltf/pltf.h"

#define GAME_VERSION_MAJ 0
#define GAME_VERSION_MIN 1

// game version - important once released
#define GAME_VERSION_GEN(P, MAJ, MIN) (u32)(((P) << 16) | ((MAJ) << 8 | (MIN)))

#if defined(PLTF_PD)
#define GAME_VERSION GAME_VERSION_GEN(1, GAME_VERSION_MAJ, GAME_VERSION_MIN) // 2: SDL
#elif defined(PLTF_SDL)
#define GAME_VERSION GAME_VERSION_GEN(2, GAME_VERSION_MAJ, GAME_VERSION_MIN) // 2: SDL
#endif

#include "core/assets.h"
#include "core/aud.h"
#include "core/gfx.h"
#include "core/inp.h"
#include "core/spm.h"
#include "textinput.h"
#include "util/easing.h"
#include "util/json.h"
#include "util/mathfunc.h"
#include "util/mem.h"
#include "util/memarena.h"
#include "util/rng.h"
#include "util/sorting.h"
#include "util/str.h"

#define FILEPATH_MAP      "assets/map/"
#define FILEPATH_SND      "assets/snd/"
#define FILEPATH_MUS      "assets/mus/"
#define FILEPATH_TEX      "assets/tex/"
#define FILEPATH_FNT      "assets/fnt/"
#define FILEPATH_DIALOG   "assets/dialog/"
#define FILEEXTENSION_AUD ".audio"
#define NUM_TILES         131072

typedef struct g_s   g_s;
typedef struct obj_s obj_s;

// handle to an object
// object pointer is valid (still exists) if:
//   o != NULL && GID == o->GID
typedef struct obj_handle_s {
    obj_s *o;
    u32    GID;
} obj_handle_s;

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

#define LEN_HERO_NAME        16
#define LEN_AREA_FILENAME    64
#define FADETICKS_AREALABEL  150
#define RENDER_PRIO_HERO     0x100
#define NUM_WORLD_ROOMS      64
#define NUM_WORLD_ROOM_TILES 256

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

enum {
    DIR_NONE  = 0,
    DIR_X_POS = 1,
    DIR_Y_POS = 2,
    DIR_X_NEG = 4,
    DIR_Y_NEG = 8,
};

static i32 direction_nearest(i32 dir, bool32 cw)
{
    if (dir == DIRECTION_NONE) return 0;
    return ((dir + (cw ? 0 : 6)) & 7) + 1;
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

typedef_struct (hitbox_s) {
    rec_i32 r;
    i32     damage;
    u32     flags;
    v2_i16  force_q8;
};

typedef_struct (map_pin_s) {
    u32    type;
    v2_i32 pos;
};

typedef_struct (time_real_s) {
    u16 h;
    u16 m;
    u16 s;
    u16 ms;
};

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
static i32 frame_from_ticks_pingpong(u32 t, u32 frames)
{
    u32 n = (frames << 1) - 1;
    u32 x = (((t % (n << 1)) * (frames - 1)) << 1) / n;
    u32 f = x % ((frames << 1) - 2);
    if (f < frames) return f;
    return (frames - (f % frames) - 2);
}

#endif