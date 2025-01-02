// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "pltf_pd.h"
#include "pltf.h"

PlaydateAPI *PD;

#define PD_NUM_MENU_ITEMS 3

enum {
    PD_MENU_ITEM_NONE,
    PD_MENU_ITEM_SIMPLE,
    PD_MENU_ITEM_OPTION,
    PD_MENU_ITEM_CHECKMARK
};

typedef struct {
    u32         type;
    PDMenuItem *itemp;
    void (*func)(void *ctx, i32 opt);
    void *ctx;
} PD_menu_item_s;

typedef struct {
    PDButtons      b;
    bool32         acc_active;
    PD_menu_item_s menu_items[PD_NUM_MENU_ITEMS];
} PD_s;

static PD_s g_PD;

void (*PD_system_error)(const char *fmt, ...);
void (*PD_system_logToConsole)(const char *fmt, ...);
void *(*PD_system_realloc)(void *ptr, size_t s);
int (*PD_system_formatString)(char **outstr, const char *fmt, ...);
void (*PD_system_getButtonState)(PDButtons *c, PDButtons *p, PDButtons *r);
void (*PD_graphics_markUpdatedRows)(int start, int end);
float (*PD_system_getElapsedTime)(void);
int (*PD_file_read)(SDFile *file, void *buf, uint len);
int (*PD_file_write)(SDFile *file, const void *buf, uint len);
void (*PD_system_getAccelerometer)(f32 *outx, f32 *outy, f32 *outz);

int pltf_pd_update(void *user);
int pltf_pd_audio(void *ctx, i16 *lbuf, i16 *rbuf, int len);

#ifdef _WINDLL
__declspec(dllexport)
#endif
int
eventHandler(PlaydateAPI *pd, PDSystemEvent event, u32 arg)
{
    switch (event) {
    case kEventInit:
        PD                          = pd;
        PD_system_error             = PD->system->error;
        PD_system_logToConsole      = PD->system->logToConsole;
        PD_system_realloc           = PD->system->realloc;
        PD_system_formatString      = PD->system->formatString;
        PD_system_getButtonState    = PD->system->getButtonState;
        PD_system_getElapsedTime    = PD->system->getElapsedTime;
        PD_graphics_markUpdatedRows = PD->graphics->markUpdatedRows;
        PD_file_read                = PD->file->read;
        PD_file_write               = PD->file->write;
        PD_system_getAccelerometer  = PD->system->getAccelerometer;
        PD->system->setUpdateCallback(pltf_pd_update, PD);
        PD->sound->addSource(pltf_pd_audio, 0, 0);
        PD->display->setRefreshRate(0.f);
        PD->system->resetElapsedTime();
        pltf_internal_init();
        break;
    case kEventTerminate: pltf_internal_close(); break;
    case kEventPause: pltf_internal_pause(); break;
    case kEventResume: pltf_internal_resume(); break;
    default: break;
    }
    return 0;
}

int pltf_pd_update(void *user)
{
    PDButtons cur;
    PD_system_getButtonState(&cur, 0, 0);
    g_PD.b |= cur;
    return pltf_internal_update();
}

int pltf_pd_audio(void *ctx, i16 *lbuf, i16 *rbuf, int len)
{
    pltf_internal_audio(lbuf, rbuf, len);
    return 1;
}

bool32 pltf_pd_reduce_flicker()
{
    return (bool32)PD->system->getReduceFlashing();
}

void pltf_pd_update_rows(i32 from_incl, i32 to_incl)
{
    PD_graphics_markUpdatedRows(from_incl, to_incl);
}

f32 pltf_pd_crank_deg()
{
    return PD->system->getCrankAngle();
}

f32 pltf_pd_crank()
{
    return (pltf_pd_crank_deg() * 0.002777778f); // 1 / 360
}

bool32 pltf_pd_crank_docked()
{
    return PD->system->isCrankDocked();
}

u32 pltf_pd_btn()
{
    PDButtons cur;
    PD_system_getButtonState(&cur, NULL, NULL);
    PDButtons b = g_PD.b | cur;
    g_PD.b      = cur;
    return (u32)b;
}

// BACKEND =====================================================================

f32 pltf_seconds()
{
    return PD_system_getElapsedTime();
}

void pltf_1bit_invert(bool32 i)
{
    PD->display->setInverted(i);
}

void *pltf_1bit_buffer()
{
    return PD->graphics->getFrame();
}

bool32 pltf_accelerometer_enabled()
{
    return g_PD.acc_active;
}

void pltf_accelerometer_set(bool32 enabled)
{
    PD->system->setPeripheralsEnabled(enabled ? kAccelerometer : kNone);
    g_PD.acc_active = enabled;
}

void pltf_accelerometer(f32 *x, f32 *y, f32 *z)
{
    if (g_PD.acc_active) {
        PD_system_getAccelerometer(x, y, z);
    } else {
        *x = 0.f, *y = 0.f, *z = 0.f;
    }
}

void pltf_debugr(i32 x, i32 y, i32 w, i32 h, u8 r, u8 g, u8 b, i32 t)
{
}

void *pltf_file_open_r(const char *path)
{
    return PD->file->open(path, kFileRead | kFileReadData);
}

void *pltf_file_open_w(const char *path)
{
    return PD->file->open(path, kFileWrite);
}

void *pltf_file_open_a(const char *path)
{
    return PD->file->open(path, kFileAppend);
}

bool32 pltf_file_close(void *f)
{
    return (PD->file->close(f) == 0);
}

bool32 pltf_file_del(const char *path)
{
    return (PD->file->unlink(path, 1) == 0);
}

i32 pltf_file_tell(void *f)
{
    return (i32)PD->file->tell(f);
}

i32 pltf_file_seek_set(void *f, i32 pos)
{
    return (i32)PD->file->seek(f, pos, SEEK_SET);
}

i32 pltf_file_seek_cur(void *f, i32 pos)
{
    return (i32)PD->file->seek(f, pos, SEEK_CUR);
}

i32 pltf_file_seek_end(void *f, i32 pos)
{
    return (i32)PD->file->seek(f, pos, SEEK_END);
}

i32 pltf_file_w(void *f, const void *buf, usize bsize)
{
    return (i32)PD_file_write(f, buf, (uint)bsize);
}

i32 pltf_file_r(void *f, void *buf, usize bsize)
{
    return (i32)PD_file_read(f, buf, (uint)bsize);
}

PD_menu_item_s *pltf_pd_try_menu_add(void (*func)(void *ctx, i32 opt), void *ctx)
{
    for (i32 n = 0; n < PD_NUM_MENU_ITEMS; n++) {
        PD_menu_item_s *i = &g_PD.menu_items[n];
        if (i->type == PD_MENU_ITEM_NONE) {
            i->func = func;
            i->ctx  = ctx;
            return i;
        }
    }
    return 0;
}

void pltf_pd_menu_cb(void *ctx)
{
    PD_menu_item_s *i = (PD_menu_item_s *)ctx;
    i32             v = pltf_pd_menu_opt_val(i);
    i->func(i->ctx, v);
}

void *pltf_pd_menu_add_opt(const char *title, const char **opt, i32 n_opt,
                           void (*func)(void *ctx, i32 opt), void *ctx)
{
    PD_menu_item_s *i = pltf_pd_try_menu_add(func, ctx);
    if (!i) return 0;

    i->type  = PD_MENU_ITEM_OPTION;
    i->itemp = PD->system->addOptionsMenuItem(title, opt, n_opt,
                                              pltf_pd_menu_cb, (void *)i);
    return i;
}

void *pltf_pd_menu_add(const char *title,
                       void (*func)(void *ctx, i32 opt), void *ctx)
{
    PD_menu_item_s *i = pltf_pd_try_menu_add(func, ctx);
    if (!i) return 0;

    i->type  = PD_MENU_ITEM_SIMPLE;
    i->itemp = PD->system->addMenuItem(title, pltf_pd_menu_cb, (void *)i);
    return i;
}

i32 pltf_pd_menu_opt_val(void *itemp)
{
    PD_menu_item_s *i = (PD_menu_item_s *)itemp;
    switch (i->type) {
    case PD_MENU_ITEM_OPTION:
    case PD_MENU_ITEM_CHECKMARK: return PD->system->getMenuItemValue(i->itemp);
    }
    return 0;
}

void pltf_pd_menu_opt_val_set(void *itemp, i32 v)
{
    PD_menu_item_s *i = (PD_menu_item_s *)itemp;
    switch (i->type) {
    case PD_MENU_ITEM_OPTION:
    case PD_MENU_ITEM_CHECKMARK: PD->system->setMenuItemValue(i->itemp, v);
    }
}

void pltf_pd_menu_rem(void *itemp)
{
    for (i32 n = 0; n < PD_NUM_MENU_ITEMS; n++) {
        PD_menu_item_s *i = &g_PD.menu_items[n];
        if (i == itemp) {
            mclr(i, sizeof(PD_menu_item_s));
            break;
        }
    }
}

void pltf_pd_menu_clr()
{
    PD->system->removeAllMenuItems();
    mclr(g_PD.menu_items, sizeof(g_PD.menu_items));
}