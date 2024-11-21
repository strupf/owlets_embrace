// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef APP_H
#define APP_H

#include "game.h"
#include "pltf/pltf.h"
#include "title.h"

typedef struct {
    g_s     game;
    title_s title;
} app_s;

extern app_s APP;

void app_init();
void app_tick();
void app_draw();
void app_close();
void app_resume();
void app_pause();
void app_audio(i16 *lbuf, i16 *rbuf, i32 len);

#endif