// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef GAMEDEF_H
#define GAMEDEF_H

#include "pltf/pltf.h"
//
#include "core/assets.h"
#include "core/aud.h"
#include "core/gfx.h"
#include "core/inp.h"
#include "core/spm.h"
#include "textinput.h"
#include "util/bitrw.h"
#include "util/easing.h"
#include "util/json.h"
#include "util/marena.h"
#include "util/mathfunc.h"
#include "util/rng.h"
#include "util/sorting.h"
#include "util/str.h"

#define GAME_DEMO  1
//
#define GAME_V_MAJ 0
#define GAME_V_MIN 1
#define GAME_V_PAT 0
#define GAME_V_DEV 0 // dev version pre-release - 0 for public release

#define GAME_VERSION_GEN(A, B, C, D) (((u32)(A) << 24) | \
                                      ((u32)(B) << 16) | \
                                      ((u32)(C) << 8) |  \
                                      ((u32)(D)))

#define GAME_VERSION GAME_VERSION_GEN(GAME_V_MAJ, \
                                      GAME_V_MIN, \
                                      GAME_V_PAT, \
                                      GAME_V_DEV)

typedef struct {
    ALIGNAS(4)
    u8 vmaj;
    u8 vmin;
    u8 vpat;
    u8 vdev;
} game_version_s;

static game_version_s game_version_decode(u32 v)
{
    game_version_s r = {0xFF & (v >> 24),
                        0xFF & (v >> 16),
                        0xFF & (v >> 8),
                        0xFF & (v >> 0)};
    return r;
}

#define GAME_TICKS_PER_SECOND PLTF_UPS
#define TICKS_FROM_MS(MS)     (((MS) * GAME_TICKS_PER_SECOND + 500) / 1000)
#define NUM_TILES             65536

typedef struct g_s   g_s;
typedef struct obj_s obj_s;

// handle to an object
// object pointer is valid (still exists) if:
//   o != NULL && GID == o->GID
typedef struct obj_handle_s {
    obj_s *o;
    u32    generation;
} obj_handle_s;

#define LEN_HERO_NAME       16
#define LEN_AREA_FILENAME   64
#define FADETICKS_AREALABEL 150

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
    DIR_NONE  = 1 << 0,
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
    case 0: return DIR_N;
    case 1: return DIR_NE;
    case 2: return DIR_E;
    case 3: return DIR_SE;
    case 4: return DIR_S;
    case 5: return DIR_SW;
    case 6: return DIR_W;
    case 7: return DIR_NW;
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

typedef struct hitbox_s {
    rec_i32 r;
    v2_i16  force_q8;
    u8      damage;
    u8      flags;
    u8      hitID;
} hitbox_s;

typedef struct time_real_s {
    ALIGNAS(4)
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