// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "sys.h"
#include "sys_backend.h"

#ifndef TARGET_EXTENSION
#define TARGET_EXTENSION 1
#endif

#include "PD/pd_api.h"

static_assert(SYS_FILE_R == (kFileRead | kFileReadData), "file mode");
static_assert(SYS_FILE_W == (kFileWrite), "file mode");

static_assert(SYS_FILE_SEEK_SET == SEEK_SET, "seek");
static_assert(SYS_FILE_SEEK_CUR == SEEK_CUR, "seek");
static_assert(SYS_FILE_SEEK_END == SEEK_END, "seek");

static_assert(SYS_INP_A == kButtonA, "input button mask A");
static_assert(SYS_INP_B == kButtonB, "input button mask B");
static_assert(SYS_INP_DPAD_L == kButtonLeft, "input button mask Dpad");
static_assert(SYS_INP_DPAD_R == kButtonRight, "input button mask Dpad");
static_assert(SYS_INP_DPAD_U == kButtonUp, "input button mask Dpad");
static_assert(SYS_INP_DPAD_D == kButtonDown, "input button mask Dpad");

PlaydateAPI *PD;
int (*PD_format_str)(char **ret, const char *format, ...);
void *(*PD_realloc)(void *ptr, size_t size);
static LCDBitmap *PD_menu_bm;
#ifndef SYS_PD_HW
void (*PD_log)(const char *fmt, ...);
#endif

static void (*PD_getButtonState)(PDButtons *a, PDButtons *b, PDButtons *c);
static float (*PD_getElapsedTime)(void);
static void (*PD_markUpdatedRows)(int a, int b);
static float (*PD_getCrankAngle)(void);
static int (*PD_isCrankDocked)(void);

#ifdef _WINDLL
__declspec(dllexport)
#endif
    int eventHandler(PlaydateAPI *pd, PDSystemEvent event, u32 arg)
{
    switch (event) {
    case kEventInit:
        PD = pd;
#ifndef SYS_PD_HW
        PD_log = PD->system->logToConsole;
#endif
        PD_format_str      = PD->system->formatString;
        PD_realloc         = PD->system->realloc;
        PD_getButtonState  = PD->system->getButtonState;
        PD_getElapsedTime  = PD->system->getElapsedTime;
        PD_markUpdatedRows = PD->graphics->markUpdatedRows;
        PD_getCrankAngle   = PD->system->getCrankAngle;
        PD_isCrankDocked   = PD->system->isCrankDocked;
        PD->system->setUpdateCallback(sys_tick, PD);
        PD->sound->addSource(sys_audio_cb, NULL, 0);
        PD->system->resetElapsedTime();
        PD->display->setRefreshRate(0.f);
        PD_menu_bm = PD->graphics->newBitmap(400, 240, kColorWhite);
        sys_init();
        break;
    case kEventTerminate:
        sys_close();
        break;
    case kEventPause:
        sys_pause();
        break;
    case kEventResume:
        sys_resume();
        break;
    case kEventKeyPressed:
    case kEventKeyReleased:
    case kEventInitLua:
    case kEventLock:
    case kEventUnlock:
    case kEventLowPower: break;
    }
    return 0;
}

int backend_inp()
{
    PDButtons b;
    PD_getButtonState(&b, NULL, NULL);
    return (int)b;
}

int backend_key(int key)
{
    return 0;
}

f32 backend_crank()
{
    return (PD_getCrankAngle() * 0.002777778f); // 1 / 360
}

int backend_crank_docked()
{
    return PD_isCrankDocked();
}

void backend_display_row_updated(int a, int b)
{
    PD_markUpdatedRows(a, b);
}

f32 backend_seconds()
{
    return PD_getElapsedTime();
}

u8 *backend_framebuffer()
{
    return PD->graphics->getFrame();
}

void *backend_file_open(const char *path, int mode)
{
    return PD->file->open(path, mode);
}

int backend_file_close(void *f)
{
    return PD->file->close((SDFile *)f);
}

int backend_file_read(void *f, void *buf, usize bufsize)
{
    return PD->file->read((SDFile *)f, buf, bufsize);
}

int backend_file_write(void *f, const void *buf, usize bufsize)
{
    return PD->file->write((SDFile *)f, buf, bufsize);
}

int backend_file_tell(void *f)
{
    return PD->file->tell((SDFile *)f);
}

int backend_file_seek(void *f, int pos, int origin)
{
    return PD->file->seek((SDFile *)f, pos, origin);
}

int backend_file_remove(const char *path)
{
    return PD->file->unlink(path, 0);
}

int backend_debug_space()
{
    return 0;
}

void backend_set_menu_image(u8 *px, int h, int wbyte)
{
    int wid, hei, byt;
    u8 *p;
    PD->graphics->getBitmapData(PD_menu_bm, &wid, &hei, &byt, NULL, &p);
    int y2 = hei < h ? hei : h;
    int b2 = byt < wbyte ? byt : wbyte;
    for (int y = 0; y < y2; y++) {
        for (int b = 0; b < b2; b++)
            p[b + y * byt] = px[b + y * wbyte];
    }
    PD->system->setMenuImage(PD_menu_bm, 0);
}

bool32 backend_reduced_flicker()
{
    return PD->system->getReduceFlashing();
}