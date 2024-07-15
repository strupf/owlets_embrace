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

#define PLTF_ENGINE_ONLY    0  // only compile and run the barebones engine?
#define PLTF_UPS            50 // ticks per second
#define PLTF_DISPLAY_W      400
#define PLTF_DISPLAY_H      240
#define PLTF_DISPLAY_WBYTES 52
#define PLTF_DISPLAY_WWORDS 13
//
#define PLTF_UPS_DT         0.0200f // elapsed seconds per update step (1/UPS)
#define PLTF_UPS_DT_TEST    0.0195f // elapsed seconds required to run a tick - improves frame skips at max FPS
#define PLTF_UPS_DT_CAP     0.0600f // max elapsed seconds

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
//
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

typedef struct {
    u8 n; // maximum 8 bits per run
    u8 b;
} mem_rle_s;

static u32 mem_compress_block(void *d, const void *s, u32 sizesrc)
{
    if (!d || !s || !sizesrc) return 0;

    u32        n   = 0;
    mem_rle_s *rle = (mem_rle_s *)((char *)d + sizeof(u32));

    for (u32 l = 0; l < sizesrc; l++) {
        const u8 b = ((const u8 *)s)[l];
        if (0 < n && rle->b == b && rle->n < 255) {
            rle->n++;
        } else {
            if (0 < n) {
                rle++;
            }
            rle->b = b;
            rle->n = 0;
            n++;
        }
    }
    *(u32 *)d = n;
    return (sizeof(u32) + sizeof(mem_rle_s) * n);
}

// buf = working memory
static void mem_decompress_block_from_file(void *f, void *d, void *buf, u32 bsize)
{
    u32 num = 0;
    pltf_file_r(f, &num, sizeof(u32));
    u32        num_blocks = bsize / (sizeof(mem_rle_s));
    mem_rle_s *rlebuf     = (mem_rle_s *)buf;
    char      *dst        = (char *)d;

    while (num) {
        u32 numb = num <= num_blocks ? num : num_blocks;
        num -= numb;
        pltf_file_r(f, rlebuf, numb * sizeof(mem_rle_s));
        for (u32 n = 0; n < numb; n++) {
            mem_rle_s rle = rlebuf[n];
            mset(dst, (char)rle.b, 1 + rle.n);
            dst += 1 + rle.n;
        }
    }
}

#endif