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

typedef struct {
    ALIGNAS(16)
    err32 err;
    void *f;
    i32   state_cur;
} app_load_s;

bool32 app_load_init(app_load_s *l);
err32  app_load_get_err(app_load_s *l);

i32 app_load_task(app_load_s *l, i32 state);
i32 app_load_taskID_beg(app_load_s *l);
i32 app_load_taskID_end(app_load_s *l);
i32 app_load_scale(app_load_s *l, i32 v);

#endif