// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SETTINGS_H
#define SETTINGS_H

#include "pltf/pltf_types.h"

enum {
    SETTINGS_MODE_NORMAL,      // 50 FPS, hi details
    SETTINGS_MODE_STREAMING,   // 50 FPS, lo details
    SETTINGS_MODE_POWER_SAVING // 40 FPS
};

enum {
    SETTINGS_ERR_OPEN     = 1 << 0,
    SETTINGS_ERR_CLOSE    = 1 << 1,
    SETTINGS_ERR_RW       = 1 << 2,
    SETTINGS_ERR_VERSION  = 1 << 3,
    SETTINGS_ERR_CHECKSUM = 1 << 4,
};

enum {
    SETTINGS_SWAP_TICKS_SHORT,
    SETTINGS_SWAP_TICKS_NORMAL,
    SETTINGS_SWAP_TICKS_LONG,
};

#define SETTINGS_VOL_MAX 16

enum {
    SETTINGS_EL_MODE,
    SETTINGS_EL_VOL_MUS,
    SETTINGS_EL_VOL_SFX,
    SETTINGS_EL_CROUCH_MODE,
    //
    NUM_SETTINGS_EL
};

typedef struct {
    ALIGNAS(32)
    u8  swap_a_b_buttons;
    u8  stereo;
    u8  mode;
    u8  vol_mus;
    u8  vol_sfx;
    u8  swap_ticks;
    u16 settings_el[NUM_SETTINGS_EL]; // settings values
} settings_s;

typedef struct {
    u8 v_curr;
    u8 v_prev;
    u8 tick;
} settings_el_s;

typedef struct {
    u8            active;
    u8            n_curr;
    u8            n_prev; // current selected setting
    u8            tick_lerp;
    settings_el_s settings_el[NUM_SETTINGS_EL]; // settings values
} settings_m_s;

extern settings_s SETTINGS;

// saves and loads global settings file to and from supplied pointer
void  settings_default(settings_s *s);
err32 settings_load(settings_s *s);
err32 settings_save(settings_s *s);

i32  settings_actual_value_of(settings_m_s *s, i32 elID);
void settings_update(settings_m_s *s);
void settings_draw(settings_m_s *s);

#endif