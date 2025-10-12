// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef VERSION_H
#define VERSION_H

#include "pltf/pltf_types.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 3
#define VERSION_PATCH 0

enum {
    VERSION_PLATFORM_DEFAULT,
    VERSION_PLATFORM_PD,
};

#if 0
#elif PLTF_PD
#define VERSION_PLATFORM VERSION_PLATFORM_PD
#else
#define VERSION_PLATFORM VERSION_PLATFORM_DEFAULT
#endif

// 00000000 00000000 00000000 00000000
// |  |         |  | |      | |______|_ [00...07] PATCH, 0...255
// |  |         |  | |______|__________ [08...15] MINOR, 0...255
// |  |         |__|___________________ [16...19] MAJOR, 0...15
// |__|________________________________ [28...31] PLATFORM

#define VERSION_NUM_MASK (((u32)1 << 20) - 1)
#define VERSION_GEN(PLATFORM, MAJOR, MINOR, PATCH) \
    (((u32)(PLATFORM) << 28) |                     \
     ((u32)(MAJOR) << 16) |                        \
     ((u32)(MINOR) << 8) |                         \
     ((u32)(PATCH) << 0))

#define VERSION VERSION_GEN(VERSION_PLATFORM, \
                            VERSION_MAJOR,    \
                            VERSION_MINOR,    \
                            VERSION_PATCH)

// get the pure version number MAJOR.MINOR.PATCH as an integer
#define VERSION_NUM_OF(V) ((V) & VERSION_NUM_MASK)
#define VERSION_NUM       VERSION_NUM_OF(VERSION)

typedef struct {
    ALIGNAS(4)
    u8 major;
    u8 minor;
    u8 patch;
    u8 platformID;
} version_s;

static version_s version_decode(u32 v)
{
    version_s r  = {0};
    r.major      = (v >> 16) & 0xF;
    r.minor      = (v >> 8) & 0xFF;
    r.patch      = (v >> 0) & 0xFF;
    r.platformID = (v >> 28) & 0xF;
    return r;
}
#endif