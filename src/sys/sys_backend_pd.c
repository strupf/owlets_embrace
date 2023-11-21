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
void (*PD_log)(const char *fmt, ...);
int (*PD_format_str)(char **ret, const char *format, ...);
void *(*PD_realloc)(void *ptr, size_t size);
static LCDBitmap *PD_menu_bm;

#ifdef _WINDLL
__declspec(dllexport)
#endif
    int eventHandler(PlaydateAPI *pd, PDSystemEvent event, u32 arg)
{
    switch (event) {
    case kEventInit:
        PD            = pd;
        PD_log        = PD->system->logToConsole;
        PD_format_str = PD->system->formatString;
        PD_realloc    = PD->system->realloc;
        PD->system->setUpdateCallback(sys_tick, PD);
        PD->sound->addSource(sys_audio_cb, NULL, 0);
        PD->system->resetElapsedTime();
        PD->display->setRefreshRate(0.f);

        PD_menu_bm = PD->graphics->newBitmap(400, 240, kColorWhite);
        PD->system->setMenuImage(PD_menu_bm, 0);
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
    PD->system->getButtonState(&b, NULL, NULL);
    return (int)b;
}

f32 backend_crank()
{
    return (PD->system->getCrankAngle() * 0.002777778f); // 1 / 360
}

int backend_crank_docked()
{
    return PD->system->isCrankDocked();
}

void backend_display_row_updated(int a, int b)
{
    PD->graphics->markUpdatedRows(a, b);
}

f32 backend_seconds()
{
    return PD->system->getElapsedTime();
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
    u8 *m;
    PD->graphics->getBitmapData(PD_menu_bm, &wid, &hei, &byt, &m, &p);

    int y2 = hei < h ? hei : h;
    int b2 = byt < wbyte ? byt : wbyte;
    for (int y = 0; y < y2; y++) {
        for (int b = 0; b < b2; b++) {
            p[b + y * byt] = px[b + y * wbyte];
        }
    }

    if (m) {
        memset(m, 0xFF, sizeof(u8) * hei * byt);
    }
}