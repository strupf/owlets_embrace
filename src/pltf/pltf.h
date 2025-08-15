// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_H
#define PLTF_H

#include "pltf/pltf_intrin.h"
#include "pltf/pltf_types.h"

#if PLTF_PD
#include "pltf/pltf_pd.h"
#else
#include "pltf/pltf_sdl.h"
#endif

#define PLTF_UPS               50 // ticks per second
#define PLTF_DISPLAY_W         400
#define PLTF_DISPLAY_H         240
#define PLTF_DISPLAY_WBYTES    52
#define PLTF_DISPLAY_WWORDS    13
#define PLTF_DISPLAY_NUM_WORDS 3120
#define PLTF_DISPLAY_BYTES     12480
//
#define PLTF_UPS_DT            0.0200f // elapsed seconds per update step (1/UPS)
#define PLTF_UPS_DT_TEST       0.0195f // elapsed seconds required to run a tick - improves frame skips at max FPS
#define PLTF_UPS_DT_CAP        0.1000f // max elapsed seconds before slowing down

#if PLTF_PD
#define PLTF_ACCELEROMETER_SUPPORT 1
#else
#define PLTF_ACCELEROMETER_SUPPORT 0
#endif

enum {
    PLTF_FPS_MODE_UNCAPPED,
    PLTF_FPS_MODE_40
};

i32    app_init();
void   app_tick();
void   app_draw();
void   app_audio(i16 *lbuf, i16 *rbuf, i32 len);
void   app_close();
void   app_pause();
void   app_resume();
void   app_mirror(b32 enable);
// to be implemented by platform
void   pltf_blit_text(char *str, i32 tile_x, i32 tile_y);
f32    pltf_seconds();
void   pltf_sync_timestep();
void   pltf_set_fps_mode(i32 fps_mode);
i32    pltf_cur_tick();
void   pltf_1bit_invert(bool32 i);
void  *pltf_1bit_buffer();
bool32 pltf_accelerometer_enabled();
void   pltf_accelerometer_set(bool32 enabled);
void   pltf_accelerometer(f32 *x, f32 *y, f32 *z);
bool32 pltf_reduce_flashing();
void   pltf_debugr(i32 x, i32 y, i32 w, i32 h, u8 r, u8 g, u8 b, i32 t);
void  *pltf_file_open_r(const char *path);
void  *pltf_file_open_w(const char *path);
void  *pltf_file_open_a(const char *path);
bool32 pltf_file_close(void *f);
bool32 pltf_file_del(const char *path);
i32    pltf_file_tell(void *f);
i32    pltf_file_seek_set(void *f, i32 pos);
i32    pltf_file_seek_cur(void *f, i32 pos);
i32    pltf_file_seek_end(void *f, i32 pos);
i32    pltf_file_w(void *f, const void *buf, usize bsize);
i32    pltf_file_r(void *f, void *buf, usize bsize);
b32    pltf_file_w_checked(void *f, const void *buf, usize bsize);
b32    pltf_file_r_checked(void *f, void *buf, usize bsize);
i32    pltf_internal_init();
i32    pltf_internal_update();
void   pltf_internal_audio(i16 *lbuf, i16 *rbuf, i32 len);
void   pltf_internal_set_fps(f32 fps);
void   pltf_internal_close();
void   pltf_internal_pause();
void   pltf_internal_resume();
void   pltf_internal_mirror(b32 enabled);
//
void  *pltf_mem_alloc_aligned(usize s, usize alignment);
void   pltf_mem_free_aligned(void *p);

#endif