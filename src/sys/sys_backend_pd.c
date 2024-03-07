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

PlaydateAPI      *PD;
static LCDBitmap *PD_menu_bm;
int (*PD_format_str)(char **ret, const char *format, ...);
void *(*PD_realloc)(void *ptr, size_t size);
void (*PD_log)(const char *fmt, ...);
//
static void (*PD_getButtonState)(PDButtons *a, PDButtons *b, PDButtons *c);
static void (*PD_markUpdatedRows)(int a, int b);
static float (*PD_getCrankAngle)(void);
static int (*PD_isCrankDocked)(void);
static float (*PD_seconds)(void);
//
static SDFile *(*PD_file_open)(const char *name, FileOptions mode);
static int (*PD_file_close)(SDFile *file);
static int (*PD_file_read)(SDFile *file, void *buf, unsigned int len);
static int (*PD_file_write)(SDFile *file, const void *buf, unsigned int len);
static int (*PD_file_tell)(SDFile *file);
static int (*PD_file_seek)(SDFile *file, int pos, int whence);
static int (*PD_file_unlink)(const char *name, int recursive);

#ifdef _WINDLL
__declspec(dllexport)
#endif
    int eventHandler(PlaydateAPI *pd, PDSystemEvent event, u32 arg)
{
    switch (event) {
    case kEventInit:
        PD                 = pd;
        PD_file_open       = PD->file->open;
        PD_file_read       = PD->file->read;
        PD_file_write      = PD->file->write;
        PD_file_close      = PD->file->close;
        PD_file_seek       = PD->file->seek;
        PD_file_tell       = PD->file->tell;
        PD_file_unlink     = PD->file->unlink;
        PD_markUpdatedRows = PD->graphics->markUpdatedRows;
        PD_log             = PD->system->logToConsole;
        PD_format_str      = PD->system->formatString;
        PD_realloc         = PD->system->realloc;
        PD_getButtonState  = PD->system->getButtonState;
        PD_getCrankAngle   = PD->system->getCrankAngle;
        PD_isCrankDocked   = PD->system->isCrankDocked;
        PD_seconds         = PD->system->getElapsedTime;
        PD->system->setUpdateCallback(sys_step, PD);
        PD->sound->addSource(sys_audio, NULL, 0);
        PD->display->setRefreshRate(0.f);
        PD->system->resetElapsedTime();
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
    return PD_seconds();
}

u32 *backend_framebuffer()
{
    return (u32 *)PD->graphics->getFrame();
}

void *backend_file_open(const char *path, int mode)
{
    return PD_file_open(path, mode);
}

int backend_file_close(void *f)
{
    return PD_file_close((SDFile *)f);
}

int backend_file_read(void *f, void *buf, usize bufsize)
{
    return PD_file_read((SDFile *)f, buf, bufsize);
}

int backend_file_write(void *f, const void *buf, usize bufsize)
{
    return PD_file_write((SDFile *)f, buf, bufsize);
}

int backend_file_tell(void *f)
{
    return PD_file_tell((SDFile *)f);
}

int backend_file_seek(void *f, int pos, int origin)
{
    return PD_file_seek((SDFile *)f, pos, origin);
}

int backend_file_remove(const char *path)
{
    return PD_file_unlink(path, 0);
}

int backend_debug_space()
{
    return 0;
}

void backend_set_menu_image(void *px, int h, int wbyte)
{
    int wid, hei, byt;
    u8 *p;
    PD->graphics->getBitmapData(PD_menu_bm, &wid, &hei, &byt, NULL, &p);
    int y2 = hei < h ? hei : h;
    int b2 = byt < wbyte ? byt : wbyte;
    for (int y = 0; y < y2; y++) {
        for (int b = 0; b < b2; b++) {
            p[b + y * byt] = ((u8 *)px)[b + y * wbyte];
        }
    }
    PD->system->setMenuImage(PD_menu_bm, 0);
}

void backend_set_FPS(int fps)
{
    if (fps < 0) return;
    PD->display->setRefreshRate((f32)fps);
}

void *backend_menu_item_add(const char *title, void (*cb)(void *arg), void *arg)
{
    PDMenuItem *mi = PD->system->addMenuItem(title, cb, arg);
    return mi;
}

void *backend_menu_checkmark_add(const char *title, int val, void (*cb)(void *arg), void *arg)
{
    PDMenuItem *mi = PD->system->addCheckmarkMenuItem(title, val, cb, arg);
    return mi;
}

bool32 backend_menu_checkmark(void *ptr)
{
    return PD->system->getMenuItemValue((PDMenuItem *)ptr);
}

void backend_menu_clr()
{
    PD->system->removeAllMenuItems();
}

void backend_set_volume(f32 vol)
{
}

void backend_display_inv(int i)
{
    PD->display->setInverted(i);
}