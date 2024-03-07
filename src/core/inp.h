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
    INP_CRANK_DOCKED = 0x80,
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

typedef struct {
    flags32 btn;
    i32     crank_q12;
} inp_state_s;

typedef struct {
    inp_state_s curr;
    inp_state_s prev;
} inp_s;

void   inp_update();
void   inp_on_resume();
inp_s  inp_state();
//
bool32 inps_pressed(inp_s i, int b);
bool32 inps_was_pressed(inp_s i, int b);
bool32 inps_just_pressed(inp_s i, int b);
bool32 inps_just_released(inp_s i, int b);
int    inps_dpad_x(inp_s i);
int    inps_dpad_y(inp_s i);

//
bool32 inp_pressed(int b);
bool32 inp_was_pressed(int b);
bool32 inp_just_pressed(int b);
bool32 inp_just_released(int b);
int    inp_dpad_x();
int    inp_dpad_y();
int    inp_dpad_dir();
int    inp_crank_q12();
int    inp_prev_crank_q12();
int    inp_crank_dt_q12();
int    inp_crank_calc_dt_q12(int ang_from, int ang_to);
int    inp_crank_docked();
int    inp_crank_was_docked();
int    inp_crank_just_docked();
int    inp_crank_just_undocked();
int    inp_debug_space();

#endif