// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SETTINGS_H
#define SETTINGS_H

#include "pltf/pltf_types.h"

enum {
    SETTINGS_MODE_NORMAL,    // 50.0 FPS, hi details
    SETTINGS_MODE_STREAMING, // 50.0 FPS, lo details
    SETTINGS_MODE_POWER_SAVING,
    SETTINGS_MODE_30_FPS
};

enum {
    SETTINGS_ERR_OPEN     = 1 << 0,
    SETTINGS_ERR_CLOSE    = 1 << 1,
    SETTINGS_ERR_RW       = 1 << 2,
    SETTINGS_ERR_VERSION  = 1 << 3,
    SETTINGS_ERR_CHECKSUM = 1 << 4,
};

#define SETTINGS_VOL_MAX          16
#define SETTINGS_SHAKE_SENS_MAX   8
#define SETTINGS_SHAKE_SMOOTH_MAX 8
#define SETTINGS_SWAP_TICKS_MIN   20
#define SETTINGS_SWAP_TICKS_MAX   40

typedef struct {
    ALIGNAS(32)
    u8 mode;
    u8 shake_sensitivity;
    u8 shake_smooth;
    u8 vol_mus;
    u8 vol_sfx;
    u8 swap_ticks;
} settings_s;

extern settings_s SETTINGS;

// saves and loads global settings file to and from supplied pointer
void  settings_default(settings_s *s);
err32 settings_load(settings_s *s);
err32 settings_save(settings_s *s);

#endif