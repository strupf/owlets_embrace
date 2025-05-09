// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef APP_H
#define APP_H

#include "core/assets.h"
#include "core/aud.h"
#include "core/spm.h"
#include "game.h"
#include "pltf/pltf.h"
#include "save.h"
#include "settings.h"
#include "settings_menu.h"
#include "title.h"
#include "util/timing.h"
#include "wad.h"

#define APP_STRUCT_ALIGNMENT 32

enum {
    APP_ST_TITLE,
    APP_ST_GAME
};

typedef struct app_s {
    ALIGNAS(APP_STRUCT_ALIGNMENT)
    wad_s           wad;
    assets_s        assets;
    spm_s           spm;
    aud_s           aud;
    g_s             game;
    title_s         title;
    settings_menu_s settings_menu;
    savefile_s      save;
    i32             state;
    marena_s        ma;
    byte            mem[MMEGABYTE(8)];
} app_s;

extern app_s *APP;

i32   app_init();
void  app_tick();
void  app_draw();
void  app_close();
void  app_resume();
void  app_pause();
void  app_audio(i16 *lbuf, i16 *rbuf, i32 len);
//
// allocate persistent memory
void *app_alloc(usize s);
void *app_alloc_aligned(usize s, usize alignment);
void *app_alloc_aligned_ctx(void *ctx, usize s, usize alignment);
#define app_alloct(T)     app_alloc_aligned(sizeof(T), ALIGNOF(T))
#define app_alloctn(T, N) app_alloc_aligned((N) * sizeof(T), ALIGNOF(T))

void app_set_mode(i32 mode); // settings mode

static inline allocator_s app_allocator()
{
    allocator_s a = {app_alloc_aligned_ctx, &APP->ma};
    return a;
}

void app_menu_callback_timing(void *ctx, i32 opt);
void app_menu_callback_map(void *ctx, i32 opt);
void app_menu_callback_settings(void *ctx, i32 opt);
void app_menu_callback_mus(void *ctx, i32 opt);
void app_menu_callback_resetsave(void *ctx, i32 opt);

#endif