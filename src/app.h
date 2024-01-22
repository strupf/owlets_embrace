// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef APP_H
#define APP_H

typedef struct {
    int x;
} app_s;

extern app_s APP;

void app_init();
void app_tick();
void app_draw();
void app_close();
void app_resume();
void app_pause();
//
void app_load_assets();

#endif