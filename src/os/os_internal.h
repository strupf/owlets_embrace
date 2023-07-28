/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#ifndef OS_INTERNAL_H
#define OS_INTERNAL_H

#include "os.h"

#define OS_DESKTOP_SCALE 1
#define OS_FPS_DELTA     0.0166667f
#define OS_DELTA_CAP     0.05f

enum {
        OS_SPMEM_SIZE         = 0x100000, // 1 MB
        OS_SPMEM_STACK_HEIGHT = 16,
        //
        OS_ASSETMEM_SIZE      = 0x100000, // 1 MB
        //
        OS_FRAMEBUFFER_SIZE   = 52 * 240,
};

typedef struct {
#if defined(TARGET_DESKTOP)
        u8        framebuffer[OS_FRAMEBUFFER_SIZE];
        Color     texpx[416 * 240];
        Texture2D tex;
        bool32    inverted;
        int       scalingmode;
        rec_i32   view;
        int       buttons;
        int       buttonsp;
#elif defined(TARGET_PD)
        u8       *framebuffer;
        PDButtons buttons;
        PDButtons buttonsp;
#endif
        bool32 crankdocked;
        bool32 crankdockedp;
        i32    crank_q16;
        i32    crankp_q16;
        float  lasttime;
        int    n_spmem;

        tex_s dst;

        tex_s tex_tab[NUM_TEXID];
        fnt_s fnt_tab[NUM_FNTID];
        snd_s snd_tab[NUM_SNDID];

        memarena_s      assetmem;
        ALIGNAS(4) char assetmem_raw[OS_ASSETMEM_SIZE];

        char           *spmemstack[OS_SPMEM_STACK_HEIGHT];
        memarena_s      spmem;
        ALIGNAS(4) char spmem_raw[OS_SPMEM_SIZE];
} os_s;

extern os_s g_os;

#endif