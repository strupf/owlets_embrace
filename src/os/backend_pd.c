// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "backend.h"

PlaydateAPI *PD;
void (*PD_log)(const char *fmt, ...);
float (*PD_elapsedtime)();

static void (*PD_display)(void);
static void (*PD_drawFPS)(int x, int y);
static void (*PD_markUpdatedRows)(int start, int end);

int os_do_tick_pd(void *userdata)
{
        return os_do_tick();
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
    int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg)
{
        switch (event) {
        case kEventInit:
                PD             = pd;
                PD_log         = PD->system->logToConsole;
                PD_elapsedtime = PD->system->getElapsedTime;
                PD->system->setUpdateCallback(os_do_tick_pd, PD);
                PD->system->resetElapsedTime();
                os_prepare();
                break;
        case kEventTerminate:
                os_backend_audio_close();
                os_backend_graphics_close();
                break;
        case kEventInitLua:
        case kEventLock:
        case kEventUnlock:
        case kEventPause:
        case kEventResume:
        case kEventKeyPressed:
        case kEventKeyReleased:
        case kEventLowPower: break;
        }
        return 0;
}

void os_backend_graphics_init()
{
        PD_display         = PD->graphics->display;
        PD_drawFPS         = PD->system->drawFPS;
        PD_markUpdatedRows = PD->graphics->markUpdatedRows;
        PD->display->setRefreshRate(0.f);
        g_os.framebuffer = PD->graphics->getFrame();
}

void os_backend_graphics_close()
{
}

void os_backend_graphics_begin()
{
        os_memclr4(g_os.framebuffer, OS_FRAMEBUFFER_SIZE);
        gfx_reset_pattern();
}

void os_backend_graphics_end()
{
        PD_markUpdatedRows(0, LCD_ROWS - 1); // mark all rows as updated
        PD_display();                        // update all rows
}

void os_backend_graphics_flip()
{
}

void os_backend_audio_init()
{
        // mus_play("assets/snd/pink.wav");
        PD->sound->addSource(os_audio_cb, NULL, 0);
}

void os_backend_audio_close()
{
        mus_close();
}