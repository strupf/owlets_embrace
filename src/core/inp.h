// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef INP_H
#define INP_H

#include "sys/sys.h"

enum {
    INP_A            = SYS_INP_A,
    INP_B            = SYS_INP_B,
    INP_DPAD_U       = SYS_INP_DPAD_U,
    INP_DPAD_D       = SYS_INP_DPAD_D,
    INP_DPAD_L       = SYS_INP_DPAD_L,
    INP_DPAD_R       = SYS_INP_DPAD_R,
    INP_CRANK_DOCKED = 128,
};

enum {
    INP_DPAD_DIR_NONE,
    INP_DPAD_DIR_N,
    INP_DPAD_DIR_S,
    INP_DPAD_DIR_E,
    INP_DPAD_DIR_W,
    INP_DPAD_DIR_NE,
    INP_DPAD_DIR_NW,
    INP_DPAD_DIR_SE,
    INP_DPAD_DIR_SW,
};

#if 0
enum {
    INP_BTN_A,
    INP_BTN_B,
    INP_BTN_UP,
    INP_BTN_DOWN,
    INP_BTN_RIGHT,
    INP_BTN_LEFT,
    INP_CRANK_DOCKED,
};
#endif

typedef struct {
    flags32 btn;
    flags32 btnp;
    i32     crank;
    i32     crankp;
} inp_s;

inp_s  inp_state();
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
int    inp_dpad_dir();
int    inp_crank_q16();
int    inp_prev_crank_q16();
int    inp_crank_dt_q16();
int    inp_crank_calc_dt_q16(int ang_from, int ang_to);
int    inp_crank_q8();
int    inp_prev_crank_q8();
int    inp_crank_dt_q8();
int    inp_crank_calc_dt_q8(int ang_from, int ang_to);
int    inp_crank_docked();
int    inp_crank_was_docked();
int    inp_crank_just_docked();
int    inp_crank_just_undocked();

int inp_debug_space();

#endif