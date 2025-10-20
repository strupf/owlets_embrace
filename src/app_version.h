// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef APP_VERSION_H
#define APP_VERSION_H

#include "pltf/pltf_types.h"

#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 3
#define APP_VERSION_PATCH 0

enum {
    APP_VERSION_PLATFORM_NULL     = 0,
    APP_VERSION_PLATFORM_PLAYDATE = 1
};

#define APP_VERSION_GEN(MAJOR, MINOR, PATCH) \
    (((u32)(MAJOR) << 16) |                  \
     ((u32)(MINOR) << 8) |                   \
     ((u32)(PATCH) << 0))

#define APP_VERSION APP_VERSION_GEN(APP_VERSION_MAJOR, \
                                    APP_VERSION_MINOR, \
                                    APP_VERSION_PATCH)

#if 1
#define APP_VERSION_PLATFORM APP_VERSION_PLATFORM_NULL
#elif PLTF_PD
#define APP_VERSION_PLATFORM APP_VERSION_PLATFORM_PLAYDATE
#else
#define APP_VERSION_PLATFORM APP_VERSION_PLATFORM_NULL
#endif

#endif