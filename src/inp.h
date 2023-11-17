// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef INP_H
#define INP_H

#include "sys/sys.h"

enum {
    INP_A      = SYS_INP_A,
    INP_B      = SYS_INP_B,
    INP_DPAD_U = SYS_INP_DPAD_U,
    INP_DPAD_D = SYS_INP_DPAD_D,
    INP_DPAD_L = SYS_INP_DPAD_L,
    INP_DPAD_R = SYS_INP_DPAD_R,

};

void   inp_update();
bool32 inp_pressed(int b);
bool32 inp_pressed_any(int b);
bool32 inp_pressed_all(int b);
bool32 inp_was_pressed(int b);
bool32 inp_was_pressed_any(int b);
bool32 inp_was_pressed_all(int b);
bool32 inp_just_pressed(int b);
bool32 inp_just_released(int b);
int    inp_dpad_x();
int    inp_dpad_y();
int    inp_crank_q16();
int    inp_prev_crank_q16();
int    inp_crank_dt_q16();
int    inp_crank_docked();
int    inp_crank_was_docked();
int    inp_crank_just_docked();
int    inp_crank_just_undocked();

int inp_debug_space();

#endif