// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "gamedef.h"

enum {
    SETTINGS_SCALING_INTEGER,
    SETTINGS_SCALING_FLOAT,
};

typedef struct {
    u32   game_version;
    bool8 reduce_flicker;
    u8    fps_cap;
    // below only for other platforms
    bool8 use_colors;
    bool8 fullscreen;
    u8    scaling;
    f32   vol;
} settings_s;

settings_s settings_default();
bool32     settings_save(settings_s s);
bool32     settings_load(settings_s *s);

#endif