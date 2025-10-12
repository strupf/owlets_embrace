// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef APP_LOAD_H
#define APP_LOAD_H

#include "pltf/pltf.h"
#include "wad.h"

enum {
    ERR_APP_LOAD_WAD = 1 << 0,
};

#define APP_LOAD_MAX_TASKS 512

enum {
    APP_LOAD_TASK_FUNCTION,
    APP_LOAD_TASK_TEX,
    APP_LOAD_TASK_SFX,
    APP_LOAD_TASK_FNT,
    APP_LOAD_TASK_ANI
};

typedef struct {
    ALIGNAS(16)
    u16 type;
    u16 critical;
    u32 assetID;
    u32 hash;
    void (*func)();
#if PLTF_DEBUG
    u8 name[32];
#endif
} app_load_task_s;

typedef struct {
    ALIGNAS(32)
    err32           err;
    void           *f;
    u16             n_task;
    u16             n_task_total;
    app_load_task_s tasklist[APP_LOAD_MAX_TASKS];
} app_load_s;

bool32 app_load_init(app_load_s *l);
bool32 app_load_tasks_timed(app_load_s *l, i32 millis_max); // loads queued tasks until loading time reached load_max or queue is empty; returns 1 if done
i32    app_load_progress_mul(app_load_s *l, i32 v);
err32  app_load_get_err(app_load_s *l);

#endif