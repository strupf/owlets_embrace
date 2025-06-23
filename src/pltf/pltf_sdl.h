// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_SDL_H
#define PLTF_SDL_H

#include "SDL2/SDL.h"
#include "pltf_types.h"
#include <stdio.h>

#define pltf_log_always printf

#if PLTF_ENABLE_LOG
#define pltf_log printf
#else
#define pltf_log(...)
#endif

void   pltf_sdl_audio_lock();
void   pltf_sdl_audio_unlock();
bool32 pltf_sdl_key(i32 k);
bool32 pltf_sdl_jkey(i32 k);
void   pltf_sdl_txt_inp_set_cb(void (*char_add)(char c, void *ctx), // callbacks for text input
                               void (*char_del)(void *ctx),
                               void (*close_inp)(void *ctx),
                               void *ctx);
void   pltf_sdl_txt_inp_clr_cb();

#define pltf_mem_alloc malloc
#define pltf_mem_free  free

#endif