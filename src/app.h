// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef APP_H
#define APP_H

#include "app_api.h"
#include "app_load.h"
#include "core/assets.h"
#include "core/aud.h"
#include "core/spm.h"
#include "credits.h"
#include "game.h"
#include "pltf/pltf.h"
#include "save_file.h"
#include "settings.h"
#include "title.h"
#include "util/timing.h"
#include "wad.h"

#if 0 < APP_VERSION_MAJOR
#if SAVE_FILE_VERSION == 0
#error save file version is set to 0
#endif
#if SETTINGS_FILE_VERSION == 0
#error settings file version is set to 0
#endif
#else
#define APP_SKIP_TO_GAME          0
#define APP_SIMULATE_LOADING_TIME 0
#endif

#if PLTF_PD_HW || 0
#define APP_LOAD_STATIC_RES_AT_ONCE 0 // startup with smooth loading bar
#else
#define APP_LOAD_STATIC_RES_AT_ONCE 0 // freeze on startup until everything is loaded (instant on PC)
#endif

enum {
    APP_ST_LOAD,
    APP_ST_LOAD_ERR,
    APP_ST_TITLE,
    APP_ST_GAME
};

enum {
    APP_FLAG_PD_MIRROR          = 1 << 0,
    APP_FLAG_PD_REDUCE_FLASHING = 1 << 5,
};

typedef struct app_s {
    ALIGNAS(32)
    g_s         game;
    title_s     title;
    save_file_s save;
    // settings_m_s sm;
    credits_s   credits;
    marena_s    ma;
    app_load_s  load;
    i32         state;
    i32         timer;
    i32         opt;
    i32         flags;
    b16         crank_requested;
    u16         crank_ui_tick;

    ALIGNAS(32)
    byte mem_app[MMEGABYTE(8)]; // permanent memory arena - static assets
} app_s;

extern app_s APP;

i32  app_init();
void app_tick();
void app_draw();
void app_close();
void app_resume();
void app_pause();
void app_audio(i16 *lbuf, i16 *rbuf, i32 len);
void app_mirror(b32 enable);

#endif