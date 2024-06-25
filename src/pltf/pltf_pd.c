// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "pltf_pd.h"
#include "pltf.h"

typedef struct {
    PDButtons b;
} PD_s;

static PD_s g_PD;

PlaydateAPI *PD;
void         (*PD_system_logToConsole)(const char *fmt, ...);
void        *(*PD_system_realloc)(void *ptr, size_t s);
int          (*PD_system_formatString)(char **outstr, const char *fmt, ...);
void         (*PD_system_getButtonState)(PDButtons *c, PDButtons *p, PDButtons *r);
void         (*PD_graphics_markUpdatedRows)(int start, int end);
float        (*PD_system_getElapsedTime)(void);
int          (*PD_file_read)(SDFile *file, void *buf, uint len);
int          (*PD_file_write)(SDFile *file, const void *buf, uint len);

int pltf_pd_update(void *user);
int pltf_pd_audio(void *ctx, i16 *lbuf, i16 *rbuf, int len);

#ifdef _WINDLL
__declspec(dllexport)
#endif
    int eventHandler(PlaydateAPI *pd, PDSystemEvent event, u32 arg)
{
    switch (event) {
    case kEventInit:
        PD                          = pd;
        PD_system_logToConsole      = PD->system->logToConsole;
        PD_system_realloc           = PD->system->realloc;
        PD_system_formatString      = PD->system->formatString;
        PD_system_getButtonState    = PD->system->getButtonState;
        PD_system_getElapsedTime    = PD->system->getElapsedTime;
        PD_graphics_markUpdatedRows = PD->graphics->markUpdatedRows;
        PD_file_read                = PD->file->read;
        PD_file_write               = PD->file->write;
        PD->system->setUpdateCallback(pltf_pd_update, PD);
        PD->sound->addSource(pltf_pd_audio, NULL, 0);
        PD->display->setRefreshRate(0.f);
        PD->system->resetElapsedTime();
        pltf_internal_init();
        break;
    case kEventTerminate:
        pltf_internal_close();
        break;
    case kEventPause:
        pltf_internal_pause();
        break;
    case kEventResume:
        pltf_internal_resume();
        break;
    default: break;
    }
    return 0;
}

int pltf_pd_update(void *user)
{
    PDButtons cur;
    PD_system_getButtonState(&cur, NULL, NULL);
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

void pltf_invert(bool32 i)
{
    PD->display->setInverted(i);
}

void pltf_1bit_invert(bool32 i)
{
    PD->display->setInverted(i);
}

void *pltf_1bit_buffer()
{
    return PD->graphics->getFrame();
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

i32 pltf_file_w(void *f, const void *buf, u32 bsize)
{
    return (i32)PD_file_write(f, buf, (uint)bsize);
}

i32 pltf_file_r(void *f, void *buf, u32 bsize)
{
    return (i32)PD_file_read(f, buf, (uint)bsize);
}