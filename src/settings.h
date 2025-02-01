// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SETTINGS_H
#define SETTINGS_H

#include "pltf/pltf_types.h"

enum {
    SETTINGS_MODE_NORMAL,
    SETTINGS_MODE_STREAMING,
};

enum {
    SETTINGS_ERR_OPEN     = 1 << 0,
    SETTINGS_ERR_CLOSE    = 1 << 1,
    SETTINGS_ERR_RW       = 1 << 2,
    SETTINGS_ERR_VERSION  = 1 << 3,
    SETTINGS_ERR_CHECKSUM = 1 << 4,
};

#define SETTINGS_TICKS_HOOK_CONTROL 15
#define SETTINGS_VOL_MAX            8
#define SETTINGS_SHAKE_SENS_MAX     8
#define SETTINGS_SHAKE_SMOOTH_MAX   8

typedef struct {
    u8  shake_sensitivity;
    u8  shake_smooth;
    i32 hook_mode;
    u8  mode;
    u8  vol_sfx;
    u8  vol_mus;
    u8  ticks_hook_hold;
} settings_s;

extern settings_s SETTINGS;

i32 settings_load();
i32 settings_save();

#endif