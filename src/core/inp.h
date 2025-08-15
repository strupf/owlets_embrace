// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef INP_H
#define INP_H

#include "pltf/pltf.h"
#include "util/mathfunc.h"

enum {
    INP_DPAD_DIR_NONE,
    INP_DPAD_DIR_N,
    INP_DPAD_DIR_S,
    INP_DPAD_DIR_E,
    INP_DPAD_DIR_W,
    INP_DPAD_DIR_NE,
    INP_DPAD_DIR_NW,
    INP_DPAD_DIR_SE,
    INP_DPAD_DIR_SW
};

enum {
#ifdef PLTF_PD
    INP_A  = PLTF_PD_BTN_A,
    INP_B  = PLTF_PD_BTN_B,
    INP_DL = PLTF_PD_BTN_DL,
    INP_DR = PLTF_PD_BTN_DR,
    INP_DU = PLTF_PD_BTN_DU,
    INP_DD = PLTF_PD_BTN_DD,
#else
    INP_A  = 1 << 0,
    INP_B  = 1 << 1,
    INP_DL = 1 << 2,
    INP_DR = 1 << 3,
    INP_DU = 1 << 4,
    INP_DD = 1 << 5,
#endif
    INP_SHAKE      = 1 << 14,
    INP_CRANK_DOCK = 1 << 15
};

typedef struct {
    ALIGNAS(4)
    u16 actions;
    u16 crank_q16;
} inp_state_s;

typedef struct {
    ALIGNAS(8)
    inp_state_s p;
    inp_state_s c;
} inp_s;

// crank wheel with discrete positions
typedef struct {
    u16 ang;     // q16
    u16 ang_off; // offset of the clicking wheel
    u8  n_segs;
    u8  n; // current segment
} inp_crank_click_s;

void   inp_update();
void   inp_on_resume();
i32    inp_x();    // [-1,+1]
i32    inp_y();    // [-1,+1]
i32    inp_xp();   // [-1,+1]
i32    inp_yp();   // [-1,+1]
v2_i32 inp_dir();  // [-1,+1]
v2_i32 inp_dirp(); // [-1,+1]
bool32 inp_btn(i32 b);
bool32 inp_btnp(i32 b);
bool32 inp_btn_jp(i32 b);
bool32 inp_btn_jr(i32 b);
i32    inp_crank_q16();      // curr crank angle in Q16, turns
i32    inp_crankp_q16();     // prev crank angle in Q16, turns
i32    inp_crank_qx(i32 q);  // curr crank angle in QXX, turns
i32    inp_crankp_qx(i32 q); // prev crank angle in QXX, turns
i32    inp_crank_dt_q16();
i32    inp_crank_calc_dt_q16(i32 ang_from, i32 ang_to);
i32    inp_crank_calc_dt_qx(i32 q, i32 ang_from, i32 ang_to);
void   inp_crank_click_init(inp_crank_click_s *c, i32 n_seg, i32 offs);
i32    inp_crank_click_turn_by(inp_crank_click_s *c, i32 dt_q16);
//
inp_s  inp_cur();
i32    inps_x(inp_s i);
i32    inps_y(inp_s i);
i32    inps_xp(inp_s i);
i32    inps_yp(inp_s i);
i32    inps_btn(inp_s i, i32 b);
i32    inps_btnp(inp_s i, i32 b);
i32    inps_btn_jp(inp_s i, i32 b);
i32    inps_btn_jr(inp_s i, i32 b);
i32    inps_crank_q16(inp_s i);
i32    inps_crankp_q16(inp_s i);
i32    inps_crankdt_q16(inp_s i);
#endif