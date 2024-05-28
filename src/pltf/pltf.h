// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_H
#define PLTF_H

#include "pltf_intrin.h"
#include "pltf_types.h"

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

void app_init();
void app_tick();
void app_draw();
void app_audio(i16 *lbuf, i16 *rbuf, i32 len);
void app_close();
void app_pause();
void app_resume();

#if defined(PLTF_PD)
#include "pltf_pd.h"

enum {
    PLTF_FILE_R = kFileRead | kFileReadData,
    PLTF_FILE_W = kFileWrite,
    PLTF_FILE_A = kFileAppend
};

#elif defined(PLTF_SDL)
#include "pltf_sdl.h"

enum {
    PLTF_FILE_R,
    PLTF_FILE_W,
    PLTF_FILE_A
};
#endif

enum {
    PLTF_FILE_SEEK_SET = SEEK_SET,
    PLTF_FILE_SEEK_CUR = SEEK_CUR,
    PLTF_FILE_SEEK_END = SEEK_END
};

// to be implemented by platform
f32    pltf_seconds();
void   pltf_1bit_invert(bool32 i);
void  *pltf_1bit_buffer();
void  *pltf_file_open(const char *path, i32 mode);
bool32 pltf_file_close(void *f);
bool32 pltf_file_del(const char *path);
i32    pltf_file_tell(void *f);
i32    pltf_file_seek(void *f, i32 pos, i32 origin);
i32    pltf_file_w(void *f, const void *buf, usize bsize);
i32    pltf_file_r(void *f, void *buf, usize bsize);
//
void   pltf_internal_init();
i32    pltf_internal_update();
void   pltf_internal_audio(i16 *lbuf, i16 *rbuf, i32 len);
void   pltf_internal_close();
void   pltf_internal_pause();
void   pltf_internal_resume();
#endif