// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "os_internal.h"

PlaydateAPI *PD;
void (*PD_log)(const char *fmt, ...);
float (*PD_elapsedtime)();
int (*PD_ftell)(SDFile *file);
int (*PD_fread)(SDFile *file, void *buf, unsigned int len);
int (*PD_fseek)(SDFile *file, int pos, int whence);
int (*PD_fwrite)(SDFile *file, const void *buf, unsigned int len);
SDFile *(*PD_fopen)(const char *path, FileOptions mode);
int (*PD_fclose)(SDFile *file);

static void (*PD_display)(void);
static void (*PD_drawFPS)(int x, int y);
static void (*PD_markUpdatedRows)(int start, int end);
static float (*PD_crank)(void);
static int (*PD_crankdocked)(void);
static void (*PD_buttonstate)(PDButtons *, PDButtons *, PDButtons *);

int os_do_tick_pd(void *userdata)
{
        return os_do_tick();
}

void os_backend_init()
{
        // graphics
        PD_display         = PD->graphics->display;
        PD_drawFPS         = PD->system->drawFPS;
        PD_markUpdatedRows = PD->graphics->markUpdatedRows;
        PD_fseek           = PD->file->seek;
        PD_ftell           = PD->file->tell;
        PD_fread           = PD->file->read;
        PD_fwrite          = PD->file->write;
        PD_fopen           = PD->file->open;
        PD_fclose          = PD->file->close;
        PD_crank           = PD->system->getCrankAngle;
        PD_crankdocked     = PD->system->isCrankDocked;
        PD_buttonstate     = PD->system->getButtonState;

        PD->display->setRefreshRate(0.f);
        g_os.framebuffer = PD->graphics->getFrame();
        PD->sound->addSource(os_audio_cb, NULL, 0);
}

void os_backend_close()
{
        mus_close();
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
                os_backend_close();
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

void os_backend_graphics_end()
{
        PD_markUpdatedRows(0, LCD_ROWS - 1); // mark all rows as updated
        PD_display();                        // update all rows
}

void os_backend_graphics_flip()
{
}

void os_backend_inp_update()
{
        g_os.buttonsp     = g_os.buttons;
        g_os.crankdockedp = g_os.crankdocked;
        PD_buttonstate(&g_os.buttons, NULL, NULL);
        g_os.crankdocked = PD_crankdocked();

        if (!g_os.crankdocked) {
                g_os.crankp_q16 = g_os.crank_q16;
                g_os.crank_q16  = (int)(PD_crank() * 182.04444f) & 0xFFFF;
        }
}

bool32 debug_inp_up() { return os_inp_pressed(INP_UP); }
bool32 debug_inp_down() { return os_inp_pressed(INP_DOWN); }
bool32 debug_inp_left() { return os_inp_pressed(INP_LEFT); }
bool32 debug_inp_right() { return os_inp_pressed(INP_RIGHT); }
bool32 debug_inp_w() { return 0; }
bool32 debug_inp_a() { return 0; }
bool32 debug_inp_s() { return 0; }
bool32 debug_inp_d() { return 0; }
bool32 debug_inp_enter() { return 0; }
bool32 debug_inp_space() { return 0; }