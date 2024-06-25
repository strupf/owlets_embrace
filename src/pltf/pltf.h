// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_H
#define PLTF_H

#include "pltf_intrin.h"
#include "pltf_types.h"

#if defined(PLTF_PD)
#include "pltf_pd.h"
#else
#include "pltf_sdl.h"
#endif

#define PLTF_UPS            50 // ticks per second
#define PLTF_DISPLAY_W      400
#define PLTF_DISPLAY_H      240
#define PLTF_DISPLAY_WBYTES 52
#define PLTF_DISPLAY_WWORDS 13
//
#define PLTF_ENGINE_ONLY    0
#define PLTF_UPS            50
#define PLTF_UPS_DT         0.0200f // elapsed seconds per update step (1/UPS)
#define PLTF_UPS_DT_TEST    0.0195f // elapsed seconds required to run an update, slightly lower
#define PLTF_UPS_DT_CAP     0.0600f // max elapsed seconds

enum {
    PLTF_FILE_MODE_R,
    PLTF_FILE_MODE_W,
    PLTF_FILE_MODE_A
};

void app_init();
void app_tick();
void app_draw();
void app_audio(i16 *lbuf, i16 *rbuf, i32 len);
void app_close();
void app_pause();
void app_resume();

// to be implemented by platform
void   pltf_blit_text(char *str, i32 tile_x, i32 tile_y);
f32    pltf_seconds();
u32    pltf_time();
void   pltf_1bit_invert(bool32 i);
void  *pltf_1bit_buffer();
void  *pltf_file_open(const char *path, i32 pltf_file_mode);
void  *pltf_file_open_r(const char *path);
void  *pltf_file_open_w(const char *path);
void  *pltf_file_open_a(const char *path);
bool32 pltf_file_close(void *f);
bool32 pltf_file_del(const char *path);
i32    pltf_file_tell(void *f);
i32    pltf_file_seek_set(void *f, i32 pos);
i32    pltf_file_seek_cur(void *f, i32 pos);
i32    pltf_file_seek_end(void *f, i32 pos);
i32    pltf_file_w(void *f, const void *buf, u32 bsize);
i32    pltf_file_r(void *f, void *buf, u32 bsize);
//
void   pltf_internal_init();
i32    pltf_internal_update();
void   pltf_internal_audio(i16 *lbuf, i16 *rbuf, i32 len);
void   pltf_internal_close();
void   pltf_internal_pause();
void   pltf_internal_resume();

#endif