// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
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

#define PLTF_ENGINE_ONLY       0  // only compile and run the barebones engine?
#define PLTF_UPS               50 // ticks per second
#define PLTF_DISPLAY_W         400
#define PLTF_DISPLAY_H         240
#define PLTF_DISPLAY_WBYTES    52
#define PLTF_DISPLAY_WWORDS    13
#define PLTF_DISPLAY_NUM_WORDS 3120
//
#define PLTF_UPS_DT            0.0200f // elapsed seconds per update step (1/UPS)
#define PLTF_UPS_DT_TEST       0.0195f // elapsed seconds required to run a tick - improves frame skips at max FPS
#define PLTF_UPS_DT_CAP        0.0600f // max elapsed seconds

#ifdef PLTF_PD
#define PLTF_ACCELEROMETER_SUPPORT 1
#else
#define PLTF_ACCELEROMETER_SUPPORT 0
#endif

enum {
    PLTF_FILE_MODE_R,
    PLTF_FILE_MODE_W,
    PLTF_FILE_MODE_A
};

#ifdef PLTF_PD
#define pltf_audio_set_volume(V)
#define pltf_audio_get_volume() 1.f
#define pltf_audio_lock()
#define pltf_audio_unlock()
#else
void pltf_audio_set_volume(f32 vol);
f32  pltf_audio_get_volume();
void pltf_audio_lock();
void pltf_audio_unlock();
#endif

void   app_init();
void   app_tick();
void   app_draw();
void   app_audio(i16 *lbuf, i16 *rbuf, i32 len);
void   app_close();
void   app_pause();
void   app_resume();
// to be implemented by platform
void   pltf_blit_text(char *str, i32 tile_x, i32 tile_y);
f32    pltf_seconds();
i32    pltf_cur_tick();
void   pltf_1bit_invert(bool32 i);
void  *pltf_1bit_buffer();
bool32 pltf_accelerometer_enabled();
void   pltf_accelerometer_set(bool32 enabled);
void   pltf_accelerometer(f32 *x, f32 *y, f32 *z);
void   pltf_debugr(i32 x, i32 y, i32 w, i32 h, u8 r, u8 g, u8 b, i32 t);
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
i32    pltf_file_w(void *f, const void *buf, usize bsize);
i32    pltf_file_r(void *f, void *buf, usize bsize);
//
void   pltf_internal_init();
i32    pltf_internal_update();
void   pltf_internal_audio(i16 *lbuf, i16 *rbuf, i32 len);
void   pltf_internal_close();
void   pltf_internal_pause();
void   pltf_internal_resume();

typedef struct {
    u8   n; // maximum 8 bits per run
    byte b;
} mem_rle_s;

static usize mem_compress(void *dst, const void *src, usize sizesrc)
{
    if (!dst || !src || !sizesrc) return 0;

    u32         n = 0;
    mem_rle_s  *r = (mem_rle_s *)((byte *)dst + sizeof(u32));
    const byte *s = (const byte *)src;

    for (u32 l = 0; l < sizesrc; l++) {
        const byte b = *s++;
        if (0 < n && r->b == b && r->n < 255) {
            r->n++;
        } else {
            if (0 < n) {
                r++;
            }
            r->b = b;
            r->n = 0;
            n++;
        }
    }
    *(u32 *)dst = n;
    return (sizeof(u32) + sizeof(mem_rle_s) * n);
}

// buf = working memory
static void mem_decompress_block_from_file(void *f, void *dst, void *buf, usize bsize)
{
    u32 num = 0;
    pltf_file_r(f, &num, sizeof(u32));

    // number of rles which can be read into the working buffer
    u32        num_blocks = (u32)(bsize / (sizeof(mem_rle_s)));
    mem_rle_s *r          = (mem_rle_s *)buf;
    byte      *d          = (byte *)dst;

    while (num) {
        u32 numb = num <= num_blocks ? num : num_blocks;
        num -= numb;
        pltf_file_r(f, r, numb * sizeof(mem_rle_s));
        for (u32 n = 0; n < numb; n++) {
            mem_rle_s rle = r[n];
            mset(d, rle.b, (usize)rle.n + 1);
            d += rle.n + 1;
        }
    }
}

#endif