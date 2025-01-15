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
#include "settings.h"
#include "title.h"
#include "wad.h"

#define APP_STRUCT_ALIGNMENT 32

enum {
    APP_ERR_MEM           = 1 << 0,
    APP_ERR_WAD_OPEN      = 1 << 1,
    APP_ERR_WAD_RW        = 1 << 2,
    APP_ERR_WAD_VERSION   = 1 << 3,
    APP_ERR_ASSETS_WAD    = 1 << 4,
    APP_ERR_ASSETS_DECODE = 1 << 5,
    APP_ERR_AUD_STREAM    = 1 << 6,
};

enum {
    APP_ST_LOAD_INIT,
    APP_ST_LOAD,
    APP_ST_GAME,
    APP_ST_TITLE,
};

typedef struct {
    ALIGNAS(APP_STRUCT_ALIGNMENT)
    wad_s    wad;
    assets_s assets;
    spm_s    spm;
    aud_s    aud;
    g_s      game;
    title_s  title;
    i32      state;
    marena_s ma;

    byte mem[MMEGABYTE(8)];
} app_s;

extern app_s *APP;

i32  app_init();
void app_tick();
void app_draw();
void app_close();
void app_resume();
void app_pause();
void app_audio(i16 *lbuf, i16 *rbuf, i32 len);

// allocate persistent memory
void *app_alloc(usize s);
void *app_alloc_aligned(usize s, usize alignment);
void *app_alloc_aligned_ctx(void *ctx, usize s, usize alignment);

i32 app_texID_create_put(i32 ID, i32 w, i32 h, b32 mask,
                         allocator_s a, tex_s *o_t);

static inline allocator_s app_allocator()
{
    allocator_s a = {app_alloc_aligned_ctx, &APP->ma};
    return a;
}

#endif