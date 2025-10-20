// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SETTINGS_H
#define SETTINGS_H

#include "pltf/pltf_types.h"

typedef struct {
    u8  swap_a_b_buttons;
    u8  stereo;
    u8  mode;
    u8  vol_mus;
    u8  vol_sfx;
    u8  swap_ticks;
    u16 settings_el[4]; // settings values
    //
    u8  unused[64]; // reserved for possible additions in the future
} settings_s;

extern settings_s SETTINGS;

void  settings_default();
err32 settings_r();
err32 settings_w();

#endif