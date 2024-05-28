// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_SDL_H
#define PLTF_SDL_H

#include "SDL2/SDL.h"
#include "pltf_types.h"
#include <stdio.h>

#define pltf_log printf

bool32 pltf_sdl_key(i32 k);
void   pltf_sdl_set_vol(f32 vol);
f32    pltf_sdl_vol();
void   pltf_sdl_audio_lock();
void   pltf_sdl_audio_unlock();

#endif