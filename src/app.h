// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef APP_H
#define APP_H

#include "game.h"
#include "pltf/pltf.h"
#include "settings.h"
#include "title.h"

typedef struct {
    u32 slomorate;
    u32 slomotick;

    game_s     game;
    title_s    title;
    settings_s settings;
} app_s;

extern app_s APP;

void app_init();
void app_tick();
void app_draw();
void app_close();
void app_resume();
void app_pause();
void app_audio(i16 *lbuf, i16 *rbuf, i32 len);
//
void app_slomo(u32 rate); // higher is slower
u32  app_get_slomo();

#endif