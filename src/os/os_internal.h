#ifndef OS_INTERNAL_H
#define OS_INTERNAL_H

#include "../game.h"
#include "os_audio.h"
#include "os_graphics.h"
#include "os_mem.h"
#include "os_types.h"

#define OS_FPS_DELTA          0.0166667f
#define OS_SPMEM_SIZE         0x100000
#define OS_SPMEM_STACK_HEIGHT 16

typedef struct {
#if defined(TARGET_DESKTOP)
        u8        framebuffer[52 * 240];
        Color     texpx[416 * 240];
        Texture2D tex;
        int       scalingmode;
        rec_i32   view;
        int       buttons;
        int       buttonsp;
#elif defined(TARGET_PD)
        u8       *framebuffer;
        PDButtons buttons;
        PDButtons buttonsp;
#endif
        bool32          crankdocked;
        bool32          crankdockedp;
        i32             crank_q16;
        i32             crankp_q16;
        float           time_acc;
        float           lasttime;
        int             n_spmem;
        char           *spmemstack[OS_SPMEM_STACK_HEIGHT];
        memarena_s      spmem;
        ALIGNAS(4) char spmem_raw[OS_SPMEM_SIZE];
} os_s;

extern os_s   g_os;
extern game_s g_gamestate;

void os_graphics_init(tex_s framebuffer);
void os_audio_init();
void os_backend_inp_update();
void os_backend_draw_begin();
void os_backend_draw_end();
void os_backend_flip();

#endif