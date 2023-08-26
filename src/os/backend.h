// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BACKEND_H
#define BACKEND_H

#include "os_internal.h"

void os_prepare();
int  os_do_tick();

void os_backend_graphics_init();
void os_backend_graphics_close();
void os_backend_audio_init();
void os_backend_audio_close();
void os_backend_inp_init();
void os_backend_inp_update();
void os_backend_graphics_begin();
void os_backend_graphics_end();
void os_backend_graphics_flip();

#endif