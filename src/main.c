/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#include "game/game.h"
#include "os/os_internal.h"

static inline void os_prepare();
static inline void os_tick();

void os_backend_graphics_init();
void os_backend_graphics_close();
void os_backend_audio_init();
void os_backend_audio_close();
void os_backend_inp_init();
void os_backend_inp_update();
void os_backend_graphics_begin();
void os_backend_graphics_end();
void os_backend_graphics_flip();

os_s   g_os;
game_s g_gamestate;

#ifdef TARGET_DESKTOP
// DESKTOP =====================================================================
int main()
{
        os_prepare();
        while (!WindowShouldClose()) {
                os_tick();
        }

        os_backend_audio_close();
        os_backend_graphics_close();
        return 0;
}
#elif defined(TARGET_PD)
// PLAYDATE ====================================================================
PlaydateAPI *PD;
void (*PD_log)(const char *fmt, ...);

int os_tick_pd(void *userdata)
{
        os_tick();
        return 1;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
    int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg)
{
        switch (event) {
        case kEventInit:
                PD     = pd;
                PD_log = PD->system->logToConsole;
                PD->system->setUpdateCallback(os_tick_pd, PD);
                PD->system->resetElapsedTime();
                os_prepare();
                break;
        }
        return 0;
}

#endif
// =============================================================================

static inline void os_tick()
{
        static float timeacc;

        float time = os_time();
        timeacc += time - g_os.lasttime;
        if (timeacc > OS_DELTA_CAP) timeacc = OS_DELTA_CAP;
        g_os.lasttime     = time;
        float timeacc_tmp = timeacc;
        while (timeacc >= OS_FPS_DELTA) {
                timeacc -= OS_FPS_DELTA;
                os_spmem_clr();
                os_backend_inp_update();
                game_update(&g_gamestate);
                g_gamestate.tick++;
                if (g_os.n_spmem > 0) {
                        PRINTF("WARNING: spmem is not reset\n_spmem");
                }
        }

        if (timeacc != timeacc_tmp) {
                os_spmem_clr();
                g_os.dst = g_os.tex_tab[0];
                os_backend_graphics_begin();
                game_draw(&g_gamestate);
                os_backend_graphics_end();
        }
        os_backend_graphics_flip();
}

static inline void os_prepare()
{
        memarena_init(&g_os.spmem, g_os.spmem_raw, OS_SPMEM_SIZE);
        memarena_init(&g_os.assetmem, g_os.assetmem_raw, OS_ASSETMEM_SIZE);
        os_backend_graphics_init();
        os_backend_audio_init();
        os_backend_inp_init();
        g_os.tex_tab[0] = (tex_s){g_os.framebuffer, 52, 400, 240};
        game_init(&g_gamestate);

        size_t sgame = sizeof(game_s) / 1024;
        size_t sos   = sizeof(os_s) / 1024;
        PRINTF("\n");
        PRINTF("Size game: %lli kb\n", sgame);
        PRINTF("Size os: %lli kb\n", sos);
        PRINTF("= %lli kb\n", sgame + sos);

        g_os.lasttime = os_time();
}